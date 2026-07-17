# Changelog

## 1.7.2

- Changed the default sampling interval from 500 ms to 50 ms.
- Reduced the minimum selectable sampling interval from 50 ms to 20 ms.
- Changed the background task to periodic scheduling with `vTaskDelayUntil()`
  so snapshot-generation time is not added to the configured interval.
- Updated all examples and documentation to use a 50-ms default interval.
- Advanced `sample_id` on every scheduled cycle so temporary BLE interruptions
  are visible as identifier gaps after reconnection.

## 1.7.1

- Removed the obsolete `esp32_live_loop()` API.
- Made background FreeRTOS acquisition and BLE transmission the only operating mode.
- Removed the unused compile-time asynchronous/synchronous mode switch.
- Updated all examples and documentation so application `loop()` contains only normal application logic.
- Renamed the internal FreeRTOS task to `esp32_live`.

## 1.7.0

- Renamed the library to ESP32 Live to match its runtime-probing purpose.
- Renamed the public header to `esp32_live.h`.
- Renamed the public functions to `esp32_live_begin()` and
  `esp32_live_is_connected()`.
- Removed the previous product term from source identifiers, metadata, examples,
  and documentation.
- Kept the BLE UUIDs and packet protocol unchanged for mobile-app compatibility.
- Retained snapshot-level `sample_id` and monotonic ESP32 timestamps.
- This release contains breaking source-level API renames.

## 1.6.4

- Added a snapshot-level `sample_id` for detecting missing acquisitions.
- Changed the runtime timestamp to monotonic ESP32 milliseconds.
- Ensured that all BLE chunks belonging to one snapshot share the same
  `sample_id` and timestamp.
- Kept the existing BLE UUIDs and public probe API unchanged.
- No breaking API changes.

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
