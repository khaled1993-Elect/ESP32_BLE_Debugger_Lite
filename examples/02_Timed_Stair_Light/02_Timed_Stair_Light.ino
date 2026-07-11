/*
  Example 2: Timed Stair Light
  ----------------------------
  Press the BOOT button to turn GPIO 2 ON for 5 seconds.

  This is similar to a stairway or corridor light timer.

  The app shows:
    GPIO 0    -> real button input
    GPIO 2    -> real light output
    Probe 200 -> lightState
    Probe 201 -> secondsRemaining
*/

#include "esp32_ble_debugger_Lite.h"

const int BUTTON_PIN = 0;
const int LIGHT_PIN = 2;

volatile float lightState = 0;
volatile float secondsRemaining = 0;

bool lastButton = HIGH;
unsigned long lightOffTime = 0;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LIGHT_PIN, OUTPUT);

  ESP32_PROBE_GPIO(BUTTON_PIN, "DIGITAL", "IN");
  ESP32_PROBE_GPIO(LIGHT_PIN, "DIGITAL", "OUT");

  ESP32_PROBE_VIRTUAL(200, lightState);
  ESP32_PROBE_VIRTUAL(201, secondsRemaining);

  esp32_ble_debugger_begin(250, "Stair-Light");
}

void loop() {
  bool button = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  // A new press starts or restarts the 5-second timer.
  if (lastButton == HIGH && button == LOW) {
    lightState = 1;
    lightOffTime = now + 5000;

    digitalWrite(LIGHT_PIN, HIGH);
    delay(50);
  }

  // Calculate how much time is left.
  if (lightState == 1 && now < lightOffTime) {
    secondsRemaining = (lightOffTime - now) / 1000.0f;
  } else {
    lightState = 0;
    secondsRemaining = 0;
    digitalWrite(LIGHT_PIN, LOW);
  }

  lastButton = button;
  esp32_ble_debugger_loop();
}
