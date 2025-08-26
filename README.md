# ESP32 BLE Debugger Lite

**ESP32 BLE Debugger Lite** is a lightweight library for adding live, Bluetooth Low Energy (BLE) debugging to your ESP32 projects.  
It allows your firmware to stream digital, analog and virtual signal data to a companion app in real time, making it easy to monitor signals, debug behaviour and visualize performance without the need for a serial connection.

## Features

- Register digital, analog or virtual probes on the fly.
- Stream values over BLE using the Nordic UART Service (NUS) UUIDs.
- Sampling interval controlled from the companion app.  The library defaults
  to a reasonable rate until the app connects and overrides it.  An optional
  asynchronous FreeRTOS task handles sampling in the background.
- Predefined lists of safe GPIOs for ESP32 and ESP32‑S3 targets.
- Separation of interface and implementation to minimise compile times.

## Supported Hardware

This library targets **ESP32** microcontrollers with BLE capability.  
It is **not** intended for ESP8266 boards.

## Installation

1. Download the latest release of this library as a ZIP file.
2. In the Arduino IDE go to **Sketch → Include Library → Add .ZIP Library…** and select the downloaded file.
3. Restart the IDE if necessary.  The library will now be available under **Sketch → Include Library**.

## Getting Started

Include the header and start the debugger in your `setup()` function.  Register any pins you want to monitor.  In synchronous mode (`ESP_LIVE_DBG_ASYNC` set to `0`), call `esp32_ble_debugger_loop()` in your `loop()` to trigger packet sending.  In asynchronous mode (the default), the library handles sampling in a background task.

```cpp
#include <esp32_ble_debugger_Lite.h>

void setup() {
  Serial.begin(115200);
  // Start the debugger.  Do not specify a sampling interval here – the
  // companion app will set it after connecting.
  esp32_ble_debugger_begin();
  
  // (Optional) automatically register all safe GPIOs
  // registerSafePins();

  // Register GPIO 2 as a digital input
  esp32_ble_debugger_register_pin(2, "DIGITAL", "IN");
}

void loop() {
  // If using synchronous mode (ESP_LIVE_DBG_ASYNC == 0), call
  // esp32_ble_debugger_loop() periodically to transmit data.
  esp32_ble_debugger_loop();
}
```

Connect to the device from your companion app to start receiving live telemetry.  
Refer to the header file for additional functions such as registering virtual probes or customizing the sampling rate.

## Examples

Several example sketches are provided under the `examples` folder to get you started quickly.  You can open these from the Arduino IDE via **File → Examples → ESP32_BLE_Debugger_Lite → …**.

The sampling interval is **not** set in any sketch.  Instead, the companion app determines the rate after connecting.  Each example demonstrates a different aspect of the library:

- **BlinkAndProbe** – Blinks an LED on GPIO 2 and reports the state of an input button on GPIO 13.
- **VirtualProbes** – Demonstrates virtual probes by streaming a sine wave and a sawtooth wave.
- **SafePinsAuto** – Automatically registers all safe GPIOs for monitoring.
 - **AnalogReadAndPWM** – Reads an analog voltage on an input-only pin, exposes it as a virtual
   probe and drives an LED using PWM.  Useful as a simple test bench for
   analog input and output.

Each example is heavily commented to illustrate best practices.

## License

This library is released under the MIT License.  See the `LICENSE` file for details.

## Author

Dr. Khaled Al Haj Ali  (<khaled.alhajali.lb@gmail.com>)
