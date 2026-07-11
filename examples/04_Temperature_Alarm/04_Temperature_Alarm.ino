/*
  Example 4: Temperature Alarm
  ----------------------------
  No temperature sensor is required.

  The program creates a simulated temperature that rises and falls.
  GPIO 2 turns ON when the temperature reaches 28 C.

  This shows how the app can help test alarm logic before
  connecting a real sensor.

  The app shows:
    GPIO 2    -> real alarm output
    Probe 200 -> temperatureC
    Probe 201 -> alarmState
*/

#include <math.h>
#include "esp32_ble_debugger_Lite.h"

const int ALARM_PIN = 2;

volatile float temperatureC = 24;
volatile float alarmState = 0;

void setup() {
  pinMode(ALARM_PIN, OUTPUT);

  ESP32_PROBE_GPIO(ALARM_PIN, "DIGITAL", "OUT");

  ESP32_PROBE_VIRTUAL(200, temperatureC);
  ESP32_PROBE_VIRTUAL(201, alarmState);

  esp32_ble_debugger_begin(250, "Temperature-Alarm");
}

void loop() {
  float timeSeconds = millis() / 1000.0f;

  // Simulated temperature: approximately 20 C to 30 C.
  temperatureC = 25.0f + 5.0f * sinf(timeSeconds * 0.4f);

  // Turn the alarm ON at 28 C or above.
  alarmState = temperatureC >= 28.0f ? 1 : 0;

  digitalWrite(ALARM_PIN, alarmState > 0.5f ? HIGH : LOW);

  esp32_ble_debugger_loop();
}
