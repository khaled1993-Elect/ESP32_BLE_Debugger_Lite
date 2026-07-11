/*
  Example 1: Button Controls a Lamp
  ---------------------------------
  Press the ESP32-S3 BOOT button to toggle GPIO 2 ON and OFF.

  The app shows:
    GPIO 0    -> real button input
    GPIO 2    -> real lamp output
    Probe 200 -> lampState
    Probe 201 -> pressCount
*/

#include "esp32_ble_debugger_Lite.h"

const int BUTTON_PIN = 0;
const int LAMP_PIN = 2;

volatile float lampState = 0;
volatile float pressCount = 0;

bool lastButton = HIGH;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LAMP_PIN, OUTPUT);

  // Monitor the real input and output pins.
  ESP32_PROBE_GPIO(BUTTON_PIN, "DIGITAL", "IN");
  ESP32_PROBE_GPIO(LAMP_PIN, "DIGITAL", "OUT");

  // Monitor internal program variables.
  ESP32_PROBE_VIRTUAL(200, lampState);
  ESP32_PROBE_VIRTUAL(201, pressCount);

  esp32_ble_debugger_begin(250, "Button-Lamp");
}

void loop() {
  bool button = digitalRead(BUTTON_PIN);

  // Detect one press: HIGH changes to LOW.
  if (lastButton == HIGH && button == LOW) {
    lampState = lampState == 0 ? 1 : 0;
    pressCount++;

    digitalWrite(LAMP_PIN, lampState > 0.5f ? HIGH : LOW);

    delay(50);  // Simple button debounce
  }

  lastButton = button;
  esp32_ble_debugger_loop();
}
