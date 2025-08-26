//
// ESP32 BLE Debugger Lite
//
// This header provides the public API and lightweight definitions for the
// ESP32 BLE debugger. To reduce compile times, the bulk of the implementation
// is defined in a separate source file (esp32_ble_debugger_Lite.cpp).

#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <vector>

// Forward declaration for the temperature sensor provided by the ESP32 SDK.
extern "C" uint8_t temprature_sens_read(void);

// Version number and options
#define ESP_LIVE_DBG_VER   "1.6"

// Enable/disable internal temperature telemetry in the JSON header
#ifndef ESP_LIVE_DBG_TEMP
#define ESP_LIVE_DBG_TEMP  1
#endif

// Maximum payload size for a single BLE notification
#define BLE_CHUNK_LIMIT    240

// Control whether the debugger runs on a background FreeRTOS task (async) or
// piggybacks on the main loop (sync). Defaults to async for responsiveness.
#ifndef ESP_LIVE_DBG_ASYNC
#define ESP_LIVE_DBG_ASYNC 1
#endif

// Minimum and maximum allowed sampling interval (ms) for the debugger. Use
// these macros to constrain user-provided rates.
#ifndef ESP_LIVE_DBG_RATE_MIN
#define ESP_LIVE_DBG_RATE_MIN 50          // very permissive lower bound
#endif
#ifndef ESP_LIVE_DBG_RATE_MAX
#define ESP_LIVE_DBG_RATE_MAX 60000       // very permissive upper bound (60 s)
#endif

// Platform detection for ESP32-S3; used to determine which pins are safe to
// probe. Adapt these definitions as new targets are added.
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
#define DBG_TARGET_S3 1
#else
#define DBG_TARGET_S3 0
#endif

// Arrays of safe GPIOs. These pins will be advertised to the host and are
// automatically registered as placeholders during initialization. Keep them
// constexpr so the header remains lightweight.
extern const uint8_t SAFE_PINS[];

// Helpers to determine which pins support GPIO or DAC functionality on the
// current target. Implementations are provided in the source file.
bool isRealGpio(uint8_t n);
bool isDacPin(uint8_t n);

/* ────────────────────────────
   DebugPin now supports floats
   ──────────────────────────── */
// A DebugPin holds metadata about a probed signal. It can represent either
// a real GPIO (hardware) or a virtual probe attached to a float variable.
struct DebugPin {
  uint8_t num;                     // pin number or virtual index
  String  cfg;                     // configuration string (e.g. "DIGITAL", "ANALOG", or "VIRTUAL")
  String  dir;                     // direction (IN/OUT) or variable name for virtual pins
  float (*getter)();               // optional callback to read a float value
  float   cur;                     // cached value (used when getter/ptr are not provided)
  bool    hasCur;                  // whether the cached value is valid
  const volatile float* ptr;       // pointer to a float variable (virtual probes)
  bool    hasPtr;                  // whether the pointer is valid
};

// Global collection of registered pins. The implementation file defines this
// vector; here it is declared for external linkage.
extern std::vector<DebugPin> pins;

// Look up a pin by its number. Returns nullptr if not registered. Defined in
// the implementation file.
DebugPin* findPin(uint8_t n);

// Register a pin with optional configuration and direction strings, and an
// optional callback to provide a custom float value. If the pin already
// exists, its metadata is updated. Implemented in the source file.
void esp32_ble_debugger_register_pin(uint8_t n,
                                     String cfg = "-", String dir = "-",
                                     float (*getter)() = nullptr);

/* ── Probes (public macros) ────────────────────────────── */
// Remove any previously defined probe macro
#ifdef ESP32_DBG_PROBE
#undef ESP32_DBG_PROBE
#endif

// Virtual probe for a float variable — captures variable name automatically
#define ESP32_PROBE_VIRTUAL(pin, var) \
    esp32_ble_debugger_probe_impl((pin), &(var), #var)

// Real GPIO probe (explicit config + direction). Use this macro to add
// hardware probes without worrying about function pointers.
#define ESP32_PROBE_GPIO(pin, cfg, dir) \
    esp32_ble_debugger_register_pin((uint8_t)(pin), String(cfg), String(dir), nullptr)

// Attach a virtual probe to a float variable. A helper used by
// ESP32_PROBE_VIRTUAL; provided here for completeness. Defined in the source.
void esp32_ble_debugger_probe_impl(uint16_t n,
                                   const volatile float* pvar,
                                   const char* varName);

// Pre-fill the pins vector with safe pins as placeholders. Declared here and
// defined in the implementation. Typically called from esp32_ble_debugger_begin().
void registerSafePins();

// Serialize the state of a DebugPin into a JSON object. Defined in the
// implementation file. Users typically do not call this directly.
void jsonAddPin(JsonObject o, const DebugPin& p);

// Add packet-level header fields to a JSON document. This includes the
// debugger version, timestamp, sampling rate, and optional temperature.
void jsonAddHeader(JsonObject d);

// Send the current state of all registered pins over BLE, splitting data
// into multiple notifications as necessary to respect BLE_CHUNK_LIMIT. This
// function is called periodically by esp32_ble_debugger_loop() or the async
// task. Defined in the source file.
void sendPkt();

// Callback class used to restart BLE advertising when the central disconnects.
class AdvCB : public BLEServerCallbacks {
public:
  void onDisconnect(BLEServer* s) override;
};

// Clamp the requested sampling rate to safe bounds and update the global
// interval. Defined in the source file.
void set_rate_clamped(uint32_t ms);

// Callback class used to process incoming writes on the control characteristic.
class CtrlCB : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic* c) override;
};

// FreeRTOS task handle and task function for asynchronous mode. These are
// declared only when ESP_LIVE_DBG_ASYNC is enabled. Definitions live in
// esp32_ble_debugger_Lite.cpp.
#if ESP_LIVE_DBG_ASYNC
extern TaskHandle_t dbgTaskHandle;
void bleDbgTask(void*);
#endif

// Initialize the BLE debugger. Optionally specify an initial sampling interval.
// This sets up BLE, starts advertising, registers characteristics, and starts
// the asynchronous task if configured.
void esp32_ble_debugger_begin(uint32_t ms = 500);

// Service the debugger. When running asynchronously this is a no-op; when
// synchronous, it checks whether it's time to send a new packet based on the
// current sampling interval. Defined in the implementation file.
void esp32_ble_debugger_loop();
