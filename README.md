# ESP32 Live

**Version 1.7.2**

ESP32 Live is an Arduino library for monitoring real ESP32 GPIO
pins and internal program variables over Bluetooth Low Energy.

It works with the ESP32-Live companion app and helps students and
developers observe program behavior without relying only on the Serial Monitor.

## Main features

- Monitor digital GPIO inputs and outputs.
- Monitor analog GPIO values.
- Monitor internal `float` variables as virtual probes.
- Change the sampling interval from the mobile app.
- Use a default interval of 50 ms, with selectable intervals from 20 ms to 60 s.
- Run BLE transmission in a background FreeRTOS task.
- Restart advertising automatically after disconnection.
- Set a custom BLE device name.
- Compatible with ArduinoJson 6 and 7.
- Validate GPIO numbers for the selected ESP32 target.

## Compatibility

### Supported

- ESP32-S3 — tested with version 1.7.2.
- Original ESP32.
- ESP32-C3 and ESP32-C6 with valid board-specific GPIO selection.

### Not supported

- ESP32-S2 — no Bluetooth radio.
- ESP8266 — no Bluetooth Low Energy support.

GPIO numbers differ between ESP32 boards. Check the pinout of the exact board
before selecting real GPIO pins.

## Requirements

- Arduino IDE
- Arduino-ESP32 Core 3.x
- ArduinoJson by Benoit Blanchon
- ESP32-Live companion app

## Installation

1. Download the library ZIP.
2. Open Arduino IDE.
3. Select **Sketch > Include Library > Add .ZIP Library...**
4. Select the downloaded ZIP.
5. Install **ArduinoJson** from Library Manager.
6. Select the correct ESP32 board and COM port.

## Quick start

This example uses the ESP32-S3 onboard BOOT button to toggle GPIO2.

```cpp
#include "esp32_live.h"

const int BUTTON_PIN = 0;
const int LAMP_PIN = 2;

volatile float lampState = 0;
volatile float pressCount = 0;

bool lastButton = HIGH;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LAMP_PIN, OUTPUT);

  ESP32_PROBE_GPIO(BUTTON_PIN, "DIGITAL", "IN");
  ESP32_PROBE_GPIO(LAMP_PIN, "DIGITAL", "OUT");

  ESP32_PROBE_VIRTUAL(200, lampState);
  ESP32_PROBE_VIRTUAL(201, pressCount);

  esp32_live_begin(50, "Button-Lamp");
}

void loop() {
  bool button = digitalRead(BUTTON_PIN);

  if (lastButton == HIGH && button == LOW) {
    lampState = lampState == 0 ? 1 : 0;
    pressCount++;

    digitalWrite(LAMP_PIN, lampState > 0.5f ? HIGH : LOW);
    delay(50);
  }

  lastButton = button;
}
```

The app displays:

```text
GPIO 0    -> real button input
GPIO 2    -> real output
Probe 200 -> lampState
Probe 201 -> pressCount
```

## Probe types

### Real GPIO

```cpp
ESP32_PROBE_GPIO(2, "DIGITAL", "OUT");
ESP32_PROBE_GPIO(4, "ANALOG", "IN");
```

### Virtual variable

```cpp
volatile float voltage = 3.1f;

ESP32_PROBE_VIRTUAL(200, voltage);
```

Virtual probe rules:

- The variable must currently be a `float`.
- Use a unique probe ID from 100 to 255.
- Keep the variable alive during the monitoring session.
- `volatile` is recommended.

## Starting ESP32 Live

```cpp
esp32_live_begin();
esp32_live_begin(50);
esp32_live_begin(50, "Student-01");
```

The default BLE device name is `ESP32-device`.

After `esp32_live_begin()` is called, acquisition and BLE transmission run
automatically in a background FreeRTOS task. No function call is required from
`loop()`.

## Included examples

1. **01_Button_Controls_Lamp**  
   Press BOOT to toggle GPIO2 and count button presses.

2. **02_Timed_Stair_Light**  
   Press BOOT to keep GPIO2 active for five seconds.

3. **03_Door_Open_Alarm**  
   Hold BOOT for three seconds to activate an alarm output.

4. **04_Temperature_Alarm**  
   Test threshold logic using a simulated temperature.

5. **05_Machine_Cycle_Monitor**  
   Start a ten-second machine cycle and monitor its progress.

These examples are designed for the ESP32-S3 board and require no external
sensors. GPIO2 is used as a real output.

## BLE protocol

The BLE protocol remains compatible with version 1.6.

- Service: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- Notify: `0000DEB1-0000-1000-8000-00805F9B34FB`
- Write: `0000DEB2-0000-1000-8000-00805F9B34FB`
- Preferred MTU: 247
- `sample_id`: increments once per scheduled acquisition cycle
- `timestamp`: monotonic ESP32 time in milliseconds
- All chunks of one snapshot carry the same `sample_id` and `timestamp`
- Temporary BLE interruptions appear as `sample_id` gaps after reconnection

## Important notes

- Internal temperature is chip temperature, not room temperature.
- Analog voltage conversion is approximate unless ADC calibration is used.
- Register probes before calling `esp32_live_begin()`.
- Pressing BOOT while the sketch runs is safe.
- Holding BOOT during reset enters download mode.

## License

MIT License.
