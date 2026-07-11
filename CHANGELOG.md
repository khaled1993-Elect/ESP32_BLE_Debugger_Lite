# Changelog

## 1.6.2

- Fixed ESP32-S3 example target detection by including `Arduino.h` before checking `CONFIG_IDF_TARGET_ESP32S3`.
- Corrected both `02_RealAndVirtual_ESP32S3` and `05_AnalogReadAndPWM_ESP32S3`.

## 1.6.2

- Replaced the obsolete internal-temperature implementation with `temperatureRead()`.
- Added Bluetooth-capability compile-time checking.
- Corrected the conservative ESP32-S3 safe-pin list.
- Added GPIO and output-capability validation.
- Restricted virtual probe IDs to 100 through 255.
- Clamped the initial and app-selected sampling rate.
- Replaced the core-pinned task with portable `xTaskCreate()`.
- Stopped notifications while no BLE client is connected.
- Added preferred MTU 247 configuration.
- Added optional custom BLE device names.
- Added ArduinoJson 6/7 compatibility.
- Updated examples for Arduino-ESP32 Core 3.x.
- Separated portable, generic-pin, and ESP32-S3-specific examples.
