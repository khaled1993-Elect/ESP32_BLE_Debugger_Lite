//
// ESP32 Live
// Version 1.7.2
//
// Public API for monitoring real ESP32 GPIOs and virtual float variables
// over Bluetooth Low Energy.
//

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "soc/soc_caps.h"

#if !defined(SOC_BT_SUPPORTED) || !SOC_BT_SUPPORTED
#error "ESP32 Live requires an ESP32 chip with Bluetooth support."
#endif

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <vector>

/* --------------------------------------------------------------------------
   Version and configuration
   -------------------------------------------------------------------------- */

#define ESP32_LIVE_VERSION "1.7.2"

// Include internal chip temperature in the JSON header when the selected
// ESP32 target provides temperatureRead(). This is chip temperature, not
// ambient room temperature.
#ifndef ESP32_LIVE_INCLUDE_CHIP_TEMP
#define ESP32_LIVE_INCLUDE_CHIP_TEMP 1
#endif

// Preferred JSON payload size for a BLE notification.
// The companion app should request an MTU of 247 after connecting.
#ifndef BLE_CHUNK_LIMIT
#define BLE_CHUNK_LIMIT 240
#endif

#ifndef ESP32_LIVE_PREFERRED_MTU
#define ESP32_LIVE_PREFERRED_MTU 247
#endif


#ifndef ESP32_LIVE_RATE_MIN
#define ESP32_LIVE_RATE_MIN 20
#endif

#ifndef ESP32_LIVE_RATE_MAX
#define ESP32_LIVE_RATE_MAX 60000
#endif

/* --------------------------------------------------------------------------
   Target identification
   -------------------------------------------------------------------------- */

#if defined(CONFIG_IDF_TARGET_ESP32S3) || \
    defined(ESP32S3) || \
    defined(ARDUINO_ESP32S3_DEV)
#define LIVE_TARGET_S3 1
#else
#define LIVE_TARGET_S3 0
#endif

#if defined(CONFIG_IDF_TARGET_ESP32)
#define LIVE_TARGET_CLASSIC_ESP32 1
#else
#define LIVE_TARGET_CLASSIC_ESP32 0
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

struct ProbeEntry {
  uint8_t num;
  String cfg;
  String dir;
  float (*getter)();
  float cur;
  bool hasCur;
  const volatile float* ptr;
  bool hasPtr;
};

extern std::vector<ProbeEntry> pins;

ProbeEntry* findPin(uint8_t n);

void esp32_live_register_pin(
    uint8_t n,
    String cfg = "-",
    String dir = "-",
    float (*getter)() = nullptr);

/* --------------------------------------------------------------------------
   Public probe macros
   -------------------------------------------------------------------------- */

// Monitor a float variable. Valid virtual IDs are 100 through 255.
#define ESP32_PROBE_VIRTUAL(id, var) \
    esp32_live_probe_impl((id), &(var), #var)

// Monitor a physical GPIO.
#define ESP32_PROBE_GPIO(pin, cfg, dir) \
    esp32_live_register_pin( \
        static_cast<uint8_t>(pin), String(cfg), String(dir), nullptr)

void esp32_live_probe_impl(
    uint16_t n,
    const volatile float* pvar,
    const char* varName);

void registerSafePins();

/* --------------------------------------------------------------------------
   Internal helpers
   -------------------------------------------------------------------------- */

void jsonAddProbe(JsonObject object, const ProbeEntry& pin);
void jsonAddHeader(JsonObject document);
void sendSnapshot();
void setSamplingIntervalClamped(uint32_t ms);

bool esp32_live_is_connected();

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
   Main API
   -------------------------------------------------------------------------- */

// Supported initialization forms:
//     esp32_live_begin();
//     esp32_live_begin(50);
//
// A custom name is useful when several student boards are nearby:
//     esp32_live_begin(50, "Student-01");
void esp32_live_begin(
    uint32_t ms = 50,
    const char* deviceName = "ESP32-device");

