# Changelog

## 1.6.3

- Corrected all version references to 1.6.3.
- Added five short beginner-friendly ESP32-S3 examples.
- Removed obsolete examples that used outdated PWM APIs or unsuitable pins.
- Kept the existing BLE UUIDs and mobile-app protocol unchanged.
- Included the ESP32-S3 compatibility and GPIO-validation corrections.
- No breaking API changes.

## 1.6.2

- Replaced the obsolete internal-temperature implementation with `temperatureRead()`.
- Added Bluetooth-capability compile-time checking.
- Corrected the conservative ESP32-S3 safe-pin list.
- Added GPIO and output-capability validation.
- Restricted virtual probe IDs to 100 through 255.
- Clamped the sampling rate.
- Replaced the core-pinned task with portable `xTaskCreate()`.
- Stopped notifications while no BLE client is connected.
- Added preferred MTU 247 configuration.
- Added optional custom BLE device names.
- Added ArduinoJson 6 and 7 compatibility.
