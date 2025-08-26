/*
  BlinkAndProbe Example
  ---------------------
  This simple sketch demonstrates how to use the ESP32 BLE Debugger Lite
  to monitor a digital input and a digital output.  An LED attached to
  GPIO2 blinks every half second, while a button on GPIO13 is reported
  as a digital input.  Connect your companion app to view these
  signals in real time.  There is no need to specify a sampling
  interval in code—the app chooses it after connecting.
*/

#include "esp32_ble_debugger_Lite.h"

void setup() {
  // Configure GPIOs
  pinMode(2, OUTPUT);
  pinMode(13, INPUT_PULLUP);

  // Register probes: input and output
  ESP32_PROBE_GPIO(13, "DIGITAL", "IN");
  ESP32_PROBE_GPIO(2,  "DIGITAL", "OUT");

  // Start BLE streaming.  Do not specify a sampling interval; the app
  // will choose it after connecting.
  esp32_ble_debugger_begin();
}

void loop() {
  static bool on = false;
  // Blink the LED
  digitalWrite(2, on ? HIGH : LOW);
  on = !on;

  // Call the debugger loop (no‑op in asynchronous mode)
  esp32_ble_debugger_loop();
  delay(500);
}