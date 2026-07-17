//
// ESP32 Live
// Version 1.7.2
//

#include "esp32_live.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <esp_timer.h>

/* --------------------------------------------------------------------------
   BLE UUIDs - unchanged for companion-app compatibility
   -------------------------------------------------------------------------- */

static const char* LIVE_SERVICE_UUID =
    "6e400001-b5a3-f393-e0a9-e50e24dcca9e";

static const char* LIVE_NOTIFY_UUID =
    "0000DEB1-0000-1000-8000-00805F9B34FB";

static const char* LIVE_WRITE_UUID =
    "0000DEB2-0000-1000-8000-00805F9B34FB";

/* --------------------------------------------------------------------------
   Conservative automatic pin lists
   -------------------------------------------------------------------------- */

#if LIVE_TARGET_S3

// ESP32-S3 conservative list.
//
// Excluded:
// - GPIO0, GPIO3, GPIO45 and GPIO46: strapping pins
// - GPIO19 and GPIO20: commonly used by native USB
// - GPIO33 through GPIO37: may be used by octal flash/PSRAM
// - GPIO43 and GPIO44: commonly used by UART0
const uint8_t SAFE_PINS[] = {
  1, 2, 4, 5, 6, 7, 8, 9, 10,
  14, 15, 16, 17, 18, 21,
  38, 39, 40, 41, 42
};

#elif LIVE_TARGET_CLASSIC_ESP32

// Original ESP32 conservative list.
// Flash pins and common strapping pins are excluded.
// GPIO34, GPIO35, GPIO36 and GPIO39 are input-only.
const uint8_t SAFE_PINS[] = {
  4, 13, 14, 16, 17, 18, 19, 21, 22, 23,
  25, 26, 27, 32, 33, 34, 35, 36, 39
};

#else

// No automatic list is supplied for other ESP32 variants because module and
// development-board wiring differs. Explicit GPIO probes still work.
const uint8_t SAFE_PINS[] = { 0 };

#endif

#if LIVE_TARGET_S3 || LIVE_TARGET_CLASSIC_ESP32
const size_t SAFE_PIN_COUNT = sizeof(SAFE_PINS) / sizeof(SAFE_PINS[0]);
#else
const size_t SAFE_PIN_COUNT = 0;
#endif

/* --------------------------------------------------------------------------
   Global state
   -------------------------------------------------------------------------- */

static volatile uint32_t samplingIntervalMs = 50;
static volatile bool deviceConnected = false;
static bool liveInitialized = false;
static uint32_t nextSampleId = 0;

std::vector<ProbeEntry> pins;

static BLECharacteristic* notifyCharacteristic = nullptr;
static BLECharacteristic* writeCharacteristic = nullptr;

static TaskHandle_t liveTaskHandle = nullptr;

/* --------------------------------------------------------------------------
   Utility helpers
   -------------------------------------------------------------------------- */

static String limitString(String value, size_t maximumLength) {
  if (value.length() > maximumLength) {
    value.remove(maximumLength);
  }
  return value;
}

static bool isOutputCapableGpio(uint8_t n) {
  if (n >= SOC_GPIO_PIN_COUNT || n >= 64) {
    return false;
  }

  const uint64_t mask =
      static_cast<uint64_t>(SOC_GPIO_VALID_OUTPUT_GPIO_MASK);

  return (mask & (1ULL << n)) != 0;
}

bool isRealGpio(uint8_t n) {
  if (n >= SOC_GPIO_PIN_COUNT || n >= 64) {
    return false;
  }

  const uint64_t mask =
      static_cast<uint64_t>(SOC_GPIO_VALID_GPIO_MASK);

  return (mask & (1ULL << n)) != 0;
}

bool isDacPin(uint8_t n) {
#if LIVE_TARGET_CLASSIC_ESP32
  return n == 25 || n == 26;
#else
  (void)n;
  return false;
#endif
}

ProbeEntry* findPin(uint8_t n) {
  for (auto& pin : pins) {
    if (pin.num == n) {
      return &pin;
    }
  }
  return nullptr;
}

/* --------------------------------------------------------------------------
   Probe registration
   -------------------------------------------------------------------------- */

void esp32_live_register_pin(
    uint8_t n,
    String cfg,
    String dir,
    float (*getter)()) {

  if (!isRealGpio(n)) {
    return;
  }

  cfg.toUpperCase();
  dir.toUpperCase();

  cfg = limitString(cfg, 16);
  dir = limitString(dir, 32);

  if (dir == "OUT" && !isOutputCapableGpio(n)) {
    return;
  }

  if (ProbeEntry* pin = findPin(n)) {
    if (cfg.length() > 0) {
      pin->cfg = cfg;
    }
    if (dir.length() > 0) {
      pin->dir = dir;
    }
    pin->getter = getter;
    return;
  }

  pins.push_back({
    n,
    cfg,
    dir,
    getter,
    0.0f,
    false,
    nullptr,
    false
  });
}

void esp32_live_probe_impl(
    uint16_t n,
    const volatile float* pvar,
    const char* varName) {

  if (n < 100 || n > 255 || pvar == nullptr) {
    return;
  }

  String name =
      varName == nullptr ? String("variable") : String(varName);

  name = limitString(name, 32);
  const uint8_t id = static_cast<uint8_t>(n);

  if (ProbeEntry* pin = findPin(id)) {
    pin->cfg = "VIRTUAL";
    pin->dir = name;
    pin->getter = nullptr;
    pin->ptr = pvar;
    pin->hasPtr = true;
    pin->hasCur = false;
    return;
  }

  pins.push_back({
    id,
    String("VIRTUAL"),
    name,
    nullptr,
    0.0f,
    false,
    pvar,
    true
  });
}

void registerSafePins() {
  for (size_t i = 0; i < SAFE_PIN_COUNT; ++i) {
    const uint8_t n = SAFE_PINS[i];

    if (findPin(n) == nullptr) {
      pins.push_back({
        n,
        String("-"),
        String("-"),
        nullptr,
        0.0f,
        false,
        nullptr,
        false
      });
    }
  }
}

/* --------------------------------------------------------------------------
   JSON generation
   -------------------------------------------------------------------------- */

void jsonAddProbe(JsonObject object, const ProbeEntry& pin) {
  const bool isVirtual = pin.cfg == "VIRTUAL";

  object["num"] = pin.num;
  object["config"] = isVirtual ? "VIRTUAL" : pin.cfg;
  object["direction"] = pin.dir;
  object["src"] =
      isVirtual ? "virtual" : (isDacPin(pin.num) ? "dac" : "hw");

  float injected = NAN;

  if (pin.getter != nullptr) {
    injected = pin.getter();
  } else if (pin.hasPtr && pin.ptr != nullptr) {
    injected = *(pin.ptr);
  } else if (pin.hasCur) {
    injected = pin.cur;
  }

  if (isVirtual) {
    const float value = isnan(injected) ? 0.0f : injected;

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.3f", value);

    object["value"] = atof(buffer);
    object["voltage"] = "-";
    return;
  }

  const bool analog = pin.cfg == "ANALOG";
  const bool output = pin.dir == "OUT";
  const bool dacPin = analog && output && isDacPin(pin.num);

  if (!analog) {
    int digitalValue;

    if (!isnan(injected)) {
      digitalValue = injected != 0.0f ? 1 : 0;
    } else {
      digitalValue = digitalRead(pin.num) ? 1 : 0;
    }

    object["value"] = digitalValue;
    object["digital"] = digitalValue;
    object["voltage"] = digitalValue ? 3.3f : 0.0f;
    return;
  }

  int analogValue;

  if (!isnan(injected)) {
    analogValue = static_cast<int>(injected);

    if (dacPin && analogValue >= 0 && analogValue <= 255) {
      analogValue *= 16;
    }
  } else {
    analogValue = analogRead(pin.num);
  }

  analogValue = constrain(analogValue, 0, 4095);

  object["value"] = analogValue;
  object["analog"] = analogValue;
  object["voltage"] = 3.3f * analogValue / 4095.0f;
}

void jsonAddHeader(JsonObject document) {
  document["ver"] = ESP32_LIVE_VERSION;
  document["timestamp"] =
      static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
  document["rate"] = samplingIntervalMs;

#if ESP32_LIVE_INCLUDE_CHIP_TEMP
  #if defined(CONFIG_IDF_TARGET_ESP32) || SOC_TEMP_SENSOR_SUPPORTED
    document["temp"] = temperatureRead();
  #endif
#endif
}

/* --------------------------------------------------------------------------
   BLE packet transmission
   -------------------------------------------------------------------------- */

bool esp32_live_is_connected() {
  return deviceConnected;
}

void sendSnapshot() {
  if (pins.empty()) {
    return;
  }

  // Advance the identifier on every scheduled acquisition cycle. If the BLE
  // link is temporarily unavailable, the next received identifier will show
  // how many complete snapshots were not delivered.
  const uint32_t sampleId = nextSampleId++;
  const uint64_t sampleTimestampMs =
      static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;

  if (!deviceConnected || notifyCharacteristic == nullptr) {
    return;
  }

  uint16_t sequence = 0;
  size_t index = 0;

  while (index < pins.size()) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument document;
#else
    StaticJsonDocument<512> document;
#endif

    jsonAddHeader(document.to<JsonObject>());
    document["sample_id"] = sampleId;
    document["timestamp"] = sampleTimestampMs;
    document["seq"] = sequence;
    document["last"] = false;

    JsonArray pinArray = document.createNestedArray("pins");

    while (index < pins.size()) {
      const size_t candidateIndex = index;

      jsonAddProbe(pinArray.createNestedObject(), pins[index]);
      ++index;

      if (measureJson(document) > BLE_CHUNK_LIMIT) {
        if (pinArray.size() > 1) {
          pinArray.remove(pinArray.size() - 1);
          index = candidateIndex;
        }
        break;
      }
    }

    document["last"] = index >= pins.size();

    char buffer[512];
    const size_t length =
        serializeJson(document, buffer, sizeof(buffer));

    if (length == 0) {
      return;
    }

    notifyCharacteristic->setValue(
        reinterpret_cast<uint8_t*>(buffer),
        length);

    notifyCharacteristic->notify();
    ++sequence;
  }
}

/* --------------------------------------------------------------------------
   BLE callbacks
   -------------------------------------------------------------------------- */

void AdvCB::onConnect(BLEServer* server) {
  (void)server;
  deviceConnected = true;
}

void AdvCB::onDisconnect(BLEServer* server) {
  deviceConnected = false;
  delay(100);
  server->startAdvertising();
}

void setSamplingIntervalClamped(uint32_t ms) {
  if (ms < ESP32_LIVE_RATE_MIN) {
    ms = ESP32_LIVE_RATE_MIN;
  }
  if (ms > ESP32_LIVE_RATE_MAX) {
    ms = ESP32_LIVE_RATE_MAX;
  }
  samplingIntervalMs = ms;
}

void CtrlCB::onWrite(BLECharacteristic* characteristic) {
  const auto raw = characteristic->getValue();
  const char* text = raw.c_str();
  const size_t length = raw.length();

  if (length == 0) {
    return;
  }

  bool containsOnlyDigitsOrWhitespace = true;

  for (size_t i = 0; i < length; ++i) {
    const unsigned char character =
        static_cast<unsigned char>(text[i]);

    if (!(isdigit(character) || isspace(character))) {
      containsOnlyDigitsOrWhitespace = false;
      break;
    }
  }

  if (containsOnlyDigitsOrWhitespace) {
    const uint32_t requestedRate =
        static_cast<uint32_t>(atoi(text));

    if (requestedRate > 0) {
      setSamplingIntervalClamped(requestedRate);
    }
    return;
  }

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument document;
#else
  StaticJsonDocument<64> document;
#endif

  const DeserializationError error =
      deserializeJson(document, text, length);

  if (error) {
    return;
  }

  if (!document["rate"].isNull()) {
    setSamplingIntervalClamped(
        static_cast<uint32_t>(document["rate"].as<int>()));
  }

}

/* --------------------------------------------------------------------------
   Background task
   -------------------------------------------------------------------------- */

static void esp32LiveTask(void*) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  uint32_t previousIntervalMs = samplingIntervalMs;

  while (true) {
    sendSnapshot();

    const uint32_t currentIntervalMs = samplingIntervalMs;

    // If the user changes the interval from the mobile application, restart
    // the periodic timing reference from the current task time.
    if (currentIntervalMs != previousIntervalMs) {
      lastWakeTime = xTaskGetTickCount();
      previousIntervalMs = currentIntervalMs;
    }

    TickType_t intervalTicks = pdMS_TO_TICKS(currentIntervalMs);
    if (intervalTicks < 1) {
      intervalTicks = 1;
    }

    // vTaskDelayUntil() makes the configured value represent the complete
    // sampling period. Snapshot-generation time is therefore not added to
    // the requested interval, unlike a delay placed after sendSnapshot().
    vTaskDelayUntil(&lastWakeTime, intervalTicks);
  }
}

/* --------------------------------------------------------------------------
   Initialization
   -------------------------------------------------------------------------- */

void esp32_live_begin(
    uint32_t ms,
    const char* deviceName) {

  setSamplingIntervalClamped(ms);

  if (liveInitialized) {
    return;
  }

  const char* resolvedName =
      (deviceName != nullptr && deviceName[0] != '\0')
          ? deviceName
          : "ESP32-device";

  BLEDevice::init(resolvedName);
  BLEDevice::setMTU(ESP32_LIVE_PREFERRED_MTU);

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new AdvCB());

  BLEService* service =
      server->createService(LIVE_SERVICE_UUID);

  notifyCharacteristic = service->createCharacteristic(
      LIVE_NOTIFY_UUID,
      BLECharacteristic::PROPERTY_NOTIFY);

  notifyCharacteristic->addDescriptor(new BLE2902());

  writeCharacteristic = service->createCharacteristic(
      LIVE_WRITE_UUID,
      BLECharacteristic::PROPERTY_WRITE);

  writeCharacteristic->setCallbacks(new CtrlCB());

  service->start();

  BLEAdvertising* advertising =
      BLEDevice::getAdvertising();

  advertising->addServiceUUID(LIVE_SERVICE_UUID);
  advertising->setScanResponse(true);

  BLEDevice::startAdvertising();

  if (liveTaskHandle == nullptr) {
    xTaskCreate(
        esp32LiveTask,
        "esp32_live",
        4096,
        nullptr,
        1,
        &liveTaskHandle);
  }

  liveInitialized = true;
}

