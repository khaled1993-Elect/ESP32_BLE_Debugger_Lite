# ESP32 BLE Debugger Lite

**Version 1.6.3**

ESP32 BLE Debugger Lite is a lightweight Arduino library for monitoring real
ESP32 GPIO pins and internal program variables over Bluetooth Low Energy.

It works with the ESP32 BLE Debugger companion app and allows students and
developers to observe program behavior without depending only on the Serial
Monitor.

## Main features

- Monitor real digital GPIO inputs and outputs.
- Monitor analog GPIO values.
- Monitor internal `float` variables as virtual probes.
- Change the sampling rate from the mobile app.
- Run BLE transmission in a background FreeRTOS task.
- Restart BLE advertising automatically after disconnection.
- Use custom BLE device names for classrooms.
- Compatible with ArduinoJson 6 and 7.
- Validate GPIO numbers for the selected ESP32 target.

## Compatibility

### Supported and tested

- **ESP32-S3**: tested target for version 1.6.3.
- **Original ESP32**: directly supported.

### Designed to work with explicit GPIO selection

- **ESP32-C3**
- **ESP32-C6**
- Other Arduino-ESP32 targets that provide Bluetooth LE and the legacy
  `BLEDevice` API.

Automatic `registerSafePins()` tables are currently supplied only for the
original ESP32 and ESP32-S3.

On ESP32-C3, ESP32-C6, or another target, select valid GPIO pins for the exact
board and register them explicitly using `ESP32_PROBE_GPIO()`.

### Not supported

- **ESP32-S2**: no Bluetooth radio.
- **ESP8266**: no Bluetooth Low Energy support.

GPIO numbers are not interchangeable between all ESP32 boards. Always check the
specific board pinout or schematic before using a real GPIO example.

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
5. Install **ArduinoJson** from the Arduino Library Manager.
6. Select the correct ESP32 board and COM port.

## Quick start

This simple example uses the ESP32-S3 onboard BOOT button to control a real
GPIO output.

Pressing the button:

- Toggles GPIO2 ON and OFF.
- Changes the `lampState` variable.
- Increases the `pressCount` variable.
- Shows all values live in the mobile app.

```cpp
#include "esp32_ble_debugger_Lite.h"

const int BUTTON_PIN = 0;
const int LAMP_PIN = 2;

volatile float lampState = 0;
volatile float pressCount = 0;

bool lastButton = HIGH;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LAMP_PIN, OUTPUT);

  // Monitor real ESP32 pins.
  ESP32_PROBE_GPIO(BUTTON_PIN, "DIGITAL", "IN");
  ESP32_PROBE_GPIO(LAMP_PIN, "DIGITAL", "OUT");

  // Monitor internal program variables.
  ESP32_PROBE_VIRTUAL(200, lampState);
  ESP32_PROBE_VIRTUAL(201, pressCount);

  esp32_ble_debugger_begin(250, "Student-Demo");
}

void loop() {
  bool button = digitalRead(BUTTON_PIN);

  // Detect one button press.
  if (lastButton == HIGH && button == LOW) {
    lampState = lampState == 0 ? 1 : 0;
    pressCount++;

    digitalWrite(LAMP_PIN, lampState > 0.5f ? HIGH : LOW);

    delay(50);  // Simple button debounce
  }

  lastButton = button;

  esp32_ble_debugger_loop();
}
```

In the app:

```text
GPIO 0    -> real button input
GPIO 2    -> real output
Probe 200 -> lampState
Probe 201 -> pressCount
```

## Probe types

### Real GPIO probe

```cpp
ESP32_PROBE_GPIO(2, "DIGITAL", "OUT");
ESP32_PROBE_GPIO(4, "ANALOG", "IN");
```

The library checks whether the GPIO exists on the selected ESP32 target.

It also rejects output mode on input-only GPIO pins.

### Virtual variable probe

```cpp
volatile float voltage = 3.1f;

ESP32_PROBE_VIRTUAL(200, voltage);
```

Virtual probe requirements:

- The variable must currently be a `float`.
- Keep the variable alive for the whole debugging session.
- Use a unique probe ID from **100 to 255**.
- `volatile` is recommended because the BLE background task reads the value.

## Starting the debugger

Use the default settings:

```cpp
esp32_ble_debugger_begin();
```

Set the sampling interval:

```cpp
esp32_ble_debugger_begin(250);
```

Set the sampling interval and BLE device name:

```cpp
esp32_ble_debugger_begin(250, "Student-07");
```

The default BLE device name is:

```text
ESP32-device
```

## Examples

### General and advanced examples

- **01_PortableVirtualSignals**  
  Demonstrates virtual signals without external hardware.

- **02_RealAndVirtual_ESP32S3**  
  Demonstrates real GPIO probes and virtual variables on ESP32-S3.

- **03_CustomRealGPIO**  
  Generic real-pin example. Change the pin constants for the selected board.

- **04_SafePinsAuto**  
  Demonstrates the conservative automatic pin table for original ESP32 and
  ESP32-S3.

- **05_AnalogReadAndPWM_ESP32S3**  
  Demonstrates ADC reading and PWM output using optional external hardware.

### Beginner application examples

- **06_Button_Controls_Lamp**  
  Press the BOOT button to toggle GPIO2 and count button presses.

- **07_Timed_Stair_Light**  
  Press the BOOT button to keep GPIO2 active for five seconds.

- **08_Door_Open_Alarm**  
  Hold the BOOT button for three seconds to activate an alarm output.

- **09_Temperature_Alarm**  
  Tests threshold and alarm logic using a simulated temperature.

- **10_Machine_Cycle_Monitor**  
  Starts a ten-second machine cycle and monitors progress and completed cycles.

## BLE protocol

The BLE UUIDs remain unchanged from version 1.6.

Service UUID:

```text
6e400001-b5a3-f393-e0a9-e50e24dcca9e
```

Notify characteristic:

```text
0000DEB1-0000-1000-8000-00805F9B34FB
```

Write characteristic:

```text
0000DEB2-0000-1000-8000-00805F9B34FB
```

The preferred MTU is 247.

The mobile app should request MTU 247 after connecting.

## Important notes

- Internal temperature is chip temperature, not room temperature.
- Analog voltage conversion is approximate unless ADC calibration is used.
- `registerSafePins()` creates monitoring placeholders only.
- `registerSafePins()` does not configure GPIO direction.
- Register all probes before calling `esp32_ble_debugger_begin()`.
- Pressing the BOOT button while the sketch is running is safe.
- Holding BOOT while resetting the ESP32 enters download mode.

## License

MIT License.
