//
// ESP32 BLE Debugger Lite
// Version 1.6.2
//
// Public API for monitoring real ESP32 GPIOs and virtual float variables
// over Bluetooth Low Energy.
//

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "soc/soc_caps.h"

#if !defined(SOC_BT_SUPPORTED) || !SOC_BT_SUPPORTED
#error "ESP32 BLE Debugger Lite requires an ESP32 chip with Bluetooth support."
#endif

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <vector>

/* --------------------------------------------------------------------------
   Version and configuration
   -------------------------------------------------------------------------- */

#define ESP_LIVE_DBG_VER "1.6.2"

// Include internal chip temperature in the JSON header when the selected
// ESP32 target provides temperatureRead(). This is chip temperature, not
// ambient room temperature.
#ifndef ESP_LIVE_DBG_TEMP
#define ESP_LIVE_DBG_TEMP 1
#endif

// Preferred JSON payload size for a BLE notification.
// The companion app should request an MTU of 247 after connecting.
#ifndef BLE_CHUNK_LIMIT
#define BLE_CHUNK_LIMIT 240
#endif

#ifndef ESP_LIVE_DBG_PREFERRED_MTU
#define ESP_LIVE_DBG_PREFERRED_MTU 247
#endif

// 1: transmit from a background FreeRTOS task.
// 0: transmit when esp32_ble_debugger_loop() is called.
#ifndef ESP_LIVE_DBG_ASYNC
#define ESP_LIVE_DBG_ASYNC 1
#endif

#ifndef ESP_LIVE_DBG_RATE_MIN
#define ESP_LIVE_DBG_RATE_MIN 50
#endif

#ifndef ESP_LIVE_DBG_RATE_MAX
#define ESP_LIVE_DBG_RATE_MAX 60000
#endif

/* --------------------------------------------------------------------------
   Target identification
   -------------------------------------------------------------------------- */

#if defined(CONFIG_IDF_TARGET_ESP32S3) || \
    defined(ESP32S3) || \
    defined(ARDUINO_ESP32S3_DEV)
#define DBG_TARGET_S3 1
#else
#define DBG_TARGET_S3 0
#endif

#if defined(CONFIG_IDF_TARGET_ESP32)
#define DBG_TARGET_CLASSIC_ESP32 1
#else
#define DBG_TARGET_CLASSIC_ESP32 0
#endif

/* --------------------------------------------------------------------------
   Probe metadata
   -------------------------------------------------------------------------- */

// Conservative automatic pin list.
// A built-in list is supplied for classic ESP32 and ESP32-S3.
// Other BLE-capable ESP32 targets should register GPIOs explicitly.
extern const uint8_t SAFE_PINS[];
extern const size_t SAFE_PIN_COUNT;

bool isRealGpio(uint8_t n);
bool isDacPin(uint8_t n);

struct DebugPin {
  uint8_t num;
  String cfg;
  String dir;
  float (*getter)();
  float cur;
  bool hasCur;
  const volatile float* ptr;
  bool hasPtr;
};

extern std::vector<DebugPin> pins;

DebugPin* findPin(uint8_t n);

void esp32_ble_debugger_register_pin(
    uint8_t n,
    String cfg = "-",
    String dir = "-",
    float (*getter)() = nullptr);

/* --------------------------------------------------------------------------
   Public probe macros
   -------------------------------------------------------------------------- */

// Monitor a float variable. Valid virtual IDs are 100 through 255.
#define ESP32_PROBE_VIRTUAL(id, var) \
    esp32_ble_debugger_probe_impl((id), &(var), #var)

// Monitor a physical GPIO.
#define ESP32_PROBE_GPIO(pin, cfg, dir) \
    esp32_ble_debugger_register_pin( \
        static_cast<uint8_t>(pin), String(cfg), String(dir), nullptr)

void esp32_ble_debugger_probe_impl(
    uint16_t n,
    const volatile float* pvar,
    const char* varName);

void registerSafePins();

/* --------------------------------------------------------------------------
   Internal helpers kept public for compatibility
   -------------------------------------------------------------------------- */

void jsonAddPin(JsonObject object, const DebugPin& pin);
void jsonAddHeader(JsonObject document);
void sendPkt();
void set_rate_clamped(uint32_t ms);

bool esp32_ble_debugger_is_connected();

/* --------------------------------------------------------------------------
   BLE callbacks
   -------------------------------------------------------------------------- */

class AdvCB : public BLEServerCallbacks {
public:
  void onConnect(BLEServer* server) override;
  void onDisconnect(BLEServer* server) override;
};

class CtrlCB : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic* characteristic) override;
};

/* --------------------------------------------------------------------------
   Background task
   -------------------------------------------------------------------------- */

#if ESP_LIVE_DBG_ASYNC
extern TaskHandle_t dbgTaskHandle;
void bleDbgTask(void*);
#endif

/* --------------------------------------------------------------------------
   Main API
   -------------------------------------------------------------------------- */

// Old calls remain valid:
//     esp32_ble_debugger_begin();
//     esp32_ble_debugger_begin(250);
//
// A custom name is useful when several student boards are nearby:
//     esp32_ble_debugger_begin(250, "Student-01");
void esp32_ble_debugger_begin(
    uint32_t ms = 500,
    const char* deviceName = "ESP32-device");

void esp32_ble_debugger_loop();
