/*
  Example 3: Door Open Alarm
  --------------------------
  Hold the BOOT button to simulate an open door.

  If the "door" stays open for 3 seconds, GPIO 2 turns ON
  to represent an alarm.

  The app shows:
    GPIO 0    -> real door input
    GPIO 2    -> real alarm output
    Probe 200 -> doorOpenSeconds
    Probe 201 -> alarmState
*/

#include "esp32_ble_debugger_Lite.h"

const int DOOR_PIN = 0;
const int ALARM_PIN = 2;

volatile float doorOpenSeconds = 0;
volatile float alarmState = 0;

unsigned long doorOpenedAt = 0;

void setup() {
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(ALARM_PIN, OUTPUT);

  ESP32_PROBE_GPIO(DOOR_PIN, "DIGITAL", "IN");
  ESP32_PROBE_GPIO(ALARM_PIN, "DIGITAL", "OUT");

  ESP32_PROBE_VIRTUAL(200, doorOpenSeconds);
  ESP32_PROBE_VIRTUAL(201, alarmState);

  esp32_ble_debugger_begin(250, "Door-Alarm");
}

void loop() {
  bool doorOpen = digitalRead(DOOR_PIN) == LOW;
  unsigned long now = millis();

  if (doorOpen) {
    // Start timing when the door first opens.
    if (doorOpenedAt == 0) {
      doorOpenedAt = now;
    }

    doorOpenSeconds = (now - doorOpenedAt) / 1000.0f;
  } else {
    // Reset everything when the door closes.
    doorOpenedAt = 0;
    doorOpenSeconds = 0;
    alarmState = 0;
  }

  // Activate the alarm after 3 seconds.
  if (doorOpenSeconds >= 3.0f) {
    alarmState = 1;
  }

  digitalWrite(ALARM_PIN, alarmState > 0.5f ? HIGH : LOW);

  esp32_ble_debugger_loop();
}
