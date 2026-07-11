# ESP32 BLE Debugger Lite

**Version 1.6.3**

ESP32 BLE Debugger Lite streams real GPIO states and virtual floating-point
variables from an ESP32 to the ESP32 BLE Debugger companion app.

## Main features

- Digital and analog real-GPIO probes.
- Virtual `float` probes for internal program variables.
- Configurable sampling rate from the app.
- Optional asynchronous FreeRTOS transmission.
- Automatic reconnection advertising.
- Custom BLE device names for classrooms.
- ArduinoJson 6 and 7 compatibility.
- GPIO validation using the selected ESP32 target.

## Compatibility

### Supported and tested

- **ESP32-S3**: tested target for version 1.6.2.
- **Original ESP32**: directly supported.

### Designed to work with explicit GPIO selection

- **ESP32-C3**
- **ESP32-C6**
- Other Arduino-ESP32 targets that provide Bluetooth LE and the legacy
  `BLEDevice` API.

Automatic `registerSafePins()` tables are currently supplied only for the
original ESP32 and ESP32-S3. On C3, C6, or another target, register valid board
pins explicitly with `ESP32_PROBE_GPIO()`.

### Not supported

- **ESP32-S2**: the chip has no Bluetooth radio.
- **ESP8266**: not an ESP32 and has no BLE support.

Pin numbers are not interchangeable among ESP32 boards. Always check the
specific board schematic before using a real GPIO example.

## Requirements

- Arduino IDE
- Arduino-ESP32 Core 3.x
- ArduinoJson by Benoit Blanchon
- ESP32 BLE Debugger companion app

## Installation

1. Download the library ZIP.
2. Open Arduino IDE.
3. Select **Sketch > Include Library > Add .ZIP Library...**
4. Select the downloaded ZIP.
5. Install **ArduinoJson** using Library Manager.
6. Select the correct ESP32 board and COM port.

## Basic example

```cpp
#include "esp32_ble_debugger_Lite.h"

volatile float temperature = 25.0f;

void setup() {
  pinMode(2, OUTPUT);

  ESP32_PROBE_GPIO(2, "DIGITAL", "OUT");
  ESP32_PROBE_VIRTUAL(200, temperature);

  esp32_ble_debugger_begin(250, "Student-01");
}

void loop() {
  temperature += 0.01f;
  digitalWrite(2, !digitalRead(2));

  esp32_ble_debugger_loop();
  delay(500);
}
```

## Probe types

### Real GPIO

```cpp
ESP32_PROBE_GPIO(2, "DIGITAL", "OUT");
ESP32_PROBE_GPIO(4, "ANALOG", "IN");
```

The GPIO must be valid for the selected chip and board. The library rejects
invalid GPIO numbers and rejects output mode on input-only pins.

### Virtual variable

```cpp
volatile float voltage = 3.1f;
ESP32_PROBE_VIRTUAL(200, voltage);
```

Virtual probe requirements:

- The variable must currently be a `float`.
- Keep the variable alive for the whole debugging session.
- Use a unique ID from **100 through 255**.
- `volatile` is recommended because the background task reads the value.

## Starting the debugger

```cpp
esp32_ble_debugger_begin();
esp32_ble_debugger_begin(250);
esp32_ble_debugger_begin(250, "Student-07");
```

The default BLE name is `ESP32-device`.

## Examples

- **01_PortableVirtualSignals**  
  No external hardware. Intended for BLE-capable ESP32 variants.

- **02_RealAndVirtual_ESP32S3**  
  Classroom/presentation example for the ESP32-S3. It monitors the onboard
  BOOT button on GPIO0, toggles GPIO2, and streams two virtual signals.

- **03_CustomRealGPIO**  
  Generic real-pin example. Edit the two pin constants to match the selected
  ESP32 development board.

- **04_SafePinsAuto**  
  Demonstrates the conservative automatic pin table. The built-in table is
  currently available only for the original ESP32 and ESP32-S3.

- **05_AnalogReadAndPWM_ESP32S3**  
  Optional hardware exercise for ESP32-S3 using an ADC input, PWM output,
  potentiometer, and external LED.

## BLE protocol

The service and characteristic UUIDs remain unchanged from version 1.6:

- Service: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- Notify: `0000DEB1-0000-1000-8000-00805F9B34FB`
- Write: `0000DEB2-0000-1000-8000-00805F9B34FB`

The preferred MTU is 247. The phone app should request MTU 247 after
connecting.

## Important notes

- Internal temperature is chip temperature, not room temperature.
- Analog voltage conversion is approximate unless ADC calibration is used.
- `registerSafePins()` does not configure the pins or determine their existing
  direction. It only creates monitoring placeholders.
- Register probes before calling `esp32_ble_debugger_begin()`.

## License

MIT License.
