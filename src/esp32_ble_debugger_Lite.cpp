// Implementation for the ESP32 BLE Debugger Lite API.
//
// Moving these definitions out of the header reduces compile times for projects
// that include the debugger. Only minimal declarations live in the header,
// while the heavy lifting lives here.

#include "esp32_ble_debugger_Lite.h"

// STL and C headers
#include <algorithm>
#include <ctype.h>  // isdigit, isspace
#include <math.h>   // isnan
#include <limits.h>

// Resolve SAFE_PINS based on the current target. These constants are not
// constexpr here to avoid duplication across translation units. If you add
// support for a new target, update these arrays accordingly.
#if DBG_TARGET_S3
// ESP32-S3 safe pins
const uint8_t SAFE_PINS[] = {
  1,2,3,4,5,6,7,8,9,10,14,15,16,17,18,21,
  33,34,35,36,37,38,39,40,41,42
};
#else
// Default ESP32 safe pins
const uint8_t SAFE_PINS[] = {
  2,12,13,14,15,36,39,34,35,32,33
};
#endif

// Global state: sampling interval and BLE characteristics. These are defined
// here rather than in the header to avoid multiple definitions when included
// from several translation units.
static uint32_t dbg_int = 500;            // default sample interval (ms)
std::vector<DebugPin> pins;               // registered debug pins
BLECharacteristic *nt = nullptr;          // notify characteristic (data out)
BLECharacteristic *wr = nullptr;          // write characteristic (commands in)

#if ESP_LIVE_DBG_ASYNC
TaskHandle_t dbgTaskHandle = nullptr;     // handle for the asynchronous task
#endif

// Implementation of helper functions. These are not marked inline here; the
// compiler may still inline them at its discretion. The definitions mirror
// those previously found in the header.
bool isRealGpio(uint8_t n) {
#if DBG_TARGET_S3
  return (n <= 21) || (n >= 26 && n <= 48);
#else
  return (n <= 39);
#endif
}

bool isDacPin(uint8_t n) {
#if DBG_TARGET_S3
  (void)n;
  return false;
#else
  return (n == 25 || n == 26);
#endif
}

// Find a registered pin by number. Returns nullptr if not found.
DebugPin* findPin(uint8_t n) {
  for (auto &p : pins) {
    if (p.num == n) {
      return &p;
    }
  }
  return nullptr;
}

// Register or update a debug pin. If the pin already exists, update its
// configuration and direction and optional getter. Otherwise, add a new entry.
void esp32_ble_debugger_register_pin(uint8_t n, String cfg, String dir,
                                     float (*getter)()) {
  if (DebugPin* p = findPin(n)) {
    if (cfg.length()) p->cfg = cfg;
    if (dir.length()) p->dir = dir;
    p->getter = getter;
  } else {
    pins.push_back({ n, cfg, dir, getter, 0.0f, false, nullptr, false });
  }
}

// Attach a virtual probe to a float variable. The pin index must be >= 100 to
// avoid clashing with real GPIO numbers. The implementation updates the
// existing entry if present.
void esp32_ble_debugger_probe_impl(uint16_t n,
                                   const volatile float* pvar,
                                   const char* varName) {
  if (n < 100) n = 100;
  DebugPin* p = findPin((uint8_t)n);
  if (!p) {
    pins.push_back({ (uint8_t)n, String("VIRTUAL"), String(varName), nullptr,
                     0.0f, false, pvar, true });
  } else {
    p->cfg    = String("VIRTUAL");
    p->dir    = String(varName);
    p->ptr    = pvar;
    p->hasPtr = true;
  }
}

// Populate the pins vector with entries for all safe GPIOs. These entries
// serve as placeholders; users can later update their configuration and
// direction when they register the pins explicitly. Avoid duplications.
void registerSafePins() {
  for (uint8_t n : SAFE_PINS) {
    if (!findPin(n)) {
      pins.push_back({ n, String("-"), String("-"), nullptr, 0.0f, false, nullptr, false });
    }
  }
}

// Serialize one pin into a JSON object. This helper populates fields based
// on the pin configuration (real vs. virtual) and current values. The same
// logic was previously inline in the header.
void jsonAddPin(JsonObject o, const DebugPin& p) {
  const bool is_virtual_pin = (p.cfg == "VIRTUAL");

  o["num"]       = p.num;
  o["config"]    = is_virtual_pin ? "VIRTUAL" : p.cfg;
  o["direction"] = p.dir;
  o["src"]       = is_virtual_pin ? "virtual" : (isDacPin(p.num) ? "dac" : "hw");

  // Determine the injected value (from getter, pointer or cached value)
  float injected = NAN;
  if (p.getter)               injected = p.getter();
  else if (p.hasPtr && p.ptr) injected = *(p.ptr);
  else if (p.hasCur)          injected = p.cur;

  if (is_virtual_pin) {
    // For virtual pins we send a floating point number with up to 3 decimals.
    float v = isnan(injected) ? 0.0f : injected;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.3f", v);
    o["value"]   = atof(buf);
    o["voltage"] = "-";
    return;
  }

  // Real GPIO handling
  const bool analog = (p.cfg == "ANALOG");
  const bool dout   = (p.dir == "OUT");
  const bool dacPin = analog && dout && isDacPin(p.num);

  if (!analog) {
    int d;
    if (!isnan(injected)) {
      d = (injected != 0.0f) ? 1 : 0;
    } else {
      d = digitalRead(p.num) ? 1 : 0;
    }
    o["value"]   = d;
    o["digital"] = d;
    o["voltage"] = d ? 3.3 : 0.0;
  } else {
    int a;
    if (!isnan(injected)) {
      a = (int)injected;
      if (dacPin && a >= 0 && a <= 255) a *= 16;  // 8-bit DAC -> 12-bit
      if (a < 0)    a = 0;
      if (a > 4095) a = 4095;
    } else {
      a = analogRead(p.num);
    }
    o["value"]   = a;
    o["analog"]  = a;
    o["voltage"] = 3.3 * a / 4095.0;
  }
}

// Add packet header fields to the JSON document. This includes version,
// timestamp, sampling rate, and optional temperature.
void jsonAddHeader(JsonObject d) {
  d["ver"]       = ESP_LIVE_DBG_VER;
  d["timestamp"] = millis();
  d["rate"]      = dbg_int;
#if ESP_LIVE_DBG_TEMP
  // Convert Fahrenheit temperature (read via ESP SDK) to Celsius
  d["temp"] = (temprature_sens_read() - 32) / 1.8;
#endif
}

// Send the current state of all registered pins over BLE. If the payload
// exceeds BLE_CHUNK_LIMIT, multiple notifications are sent with a sequence
// number and a 'last' flag to indicate completion.
void sendPkt() {
  uint16_t seq = 0;
  size_t   idx = 0;
  while (idx < pins.size()) {
    StaticJsonDocument<512> doc;
    jsonAddHeader(doc.to<JsonObject>());
    doc["seq"]  = seq;
    doc["last"] = false;
    JsonArray pinArr = doc.createNestedArray("pins");
    while (idx < pins.size()) {
      jsonAddPin(pinArr.createNestedObject(), pins[idx++]);
      if (measureJson(doc) >= BLE_CHUNK_LIMIT) {
        // Remove the last pin and send the current packet
        pinArr.remove(pinArr.size() - 1);
        --idx;
        break;
      }
    }
    if (idx >= pins.size()) {
      doc["last"] = true;
    }
    char buf[512];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    nt->setValue((uint8_t*)buf, n);
    nt->notify();
    seq++;
  }
}

// Implementation of BLE server callback: restart advertising after disconnect.
void AdvCB::onDisconnect(BLEServer* s) {
  // Give the stack some time before restarting advertising
  delay(100);
  s->startAdvertising();
}

// Clamp the sampling interval to safe bounds and update dbg_int.
void set_rate_clamped(uint32_t ms) {
  if (ms < ESP_LIVE_DBG_RATE_MIN) ms = ESP_LIVE_DBG_RATE_MIN;
  if (ms > ESP_LIVE_DBG_RATE_MAX) ms = ESP_LIVE_DBG_RATE_MAX;
  dbg_int = ms;
}

// Handle writes from the phone/app to adjust the sampling rate. Accepts
// either plain numbers or tiny JSON bodies (e.g. {"rate":100}).
void CtrlCB::onWrite(BLECharacteristic* c) {
  auto raw = c->getValue();
  const char* s = raw.c_str();
  size_t n = raw.length();
  if (n == 0) return;

  // Fast path: treat input as an integer
  bool allDigitsOrWs = true;
  for (size_t i = 0; i < n; ++i) {
    unsigned char ch = static_cast<unsigned char>(s[i]);
    if (!(isdigit(ch) || isspace(ch))) {
      allDigitsOrWs = false;
      break;
    }
  }
  if (allDigitsOrWs) {
    uint32_t ms = (uint32_t)atoi(s);
    if (ms) set_rate_clamped(ms);
    return;
  }

  // Fallback: parse tiny JSON
  StaticJsonDocument<64> doc;
  DeserializationError err = deserializeJson(doc, s, n);
  if (!err) {
    if (doc.containsKey("rate")) {
      set_rate_clamped((uint32_t)doc["rate"].as<int>());
    }
    if (doc.containsKey("dbg_int")) {
      set_rate_clamped((uint32_t)doc["dbg_int"].as<int>());
    }
  }
}

#if ESP_LIVE_DBG_ASYNC
// Asynchronous task that periodically sends pin data according to dbg_int.
void bleDbgTask(void*) {
  while (true) {
    sendPkt();
    vTaskDelay(pdMS_TO_TICKS(dbg_int));
  }
}
#endif

// Initialize BLE and start the debugger service. Optionally specify an
// initial sampling interval. This sets up characteristics, advertising and
// starts the asynchronous task if configured.
void esp32_ble_debugger_begin(uint32_t ms) {
  // Note: registerSafePins() is intentionally not called here to give the
  // application control over whether to pre-populate the pin list. If you
  // wish to enable it, call registerSafePins() before begin().
  dbg_int = ms;

  BLEDevice::init("ESP32-device");
  BLEServer* srv = BLEDevice::createServer();
  srv->setCallbacks(new AdvCB());

  BLEService* svc = srv->createService("6e400001-b5a3-f393-e0a9-e50e24dcca9e");

  // Notify characteristic (state out)
  nt = svc->createCharacteristic(
           "0000DEB1-0000-1000-8000-00805F9B34FB",
           BLECharacteristic::PROPERTY_NOTIFY);
  nt->addDescriptor(new BLE2902());

  // Write characteristic (control in)
  wr = svc->createCharacteristic(
           "0000DEB2-0000-1000-8000-00805F9B34FB",
           BLECharacteristic::PROPERTY_WRITE);
  wr->setCallbacks(new CtrlCB());

  svc->start();

  // Start advertising with the service UUID
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();

#if ESP_LIVE_DBG_ASYNC
  // Launch the asynchronous task pinned to core 1. Use a modest stack size.
  xTaskCreatePinnedToCore(bleDbgTask, "ble_dbg", 4096, nullptr, 1, &dbgTaskHandle, 1);
#endif
}

// Service the debugger. In asynchronous mode this does nothing; in
// synchronous mode (ESP_LIVE_DBG_ASYNC==0) it checks whether it's time to
// send a packet based on dbg_int.
void esp32_ble_debugger_loop() {
#if ESP_LIVE_DBG_ASYNC
  // No-op: the background task handles periodic sends
  (void)0;
#else
  static uint32_t last = 0;
  if (millis() - last >= dbg_int) {
    last = millis();
    sendPkt();
  }
#endif
}