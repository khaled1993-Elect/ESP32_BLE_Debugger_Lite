/*
  Example 5: Machine Cycle Monitor
  --------------------------------
  Press the BOOT button to start a 10-second machine cycle.

  During the cycle:
    - GPIO 2 toggles to show activity
    - progressPercent increases from 0 to 100

  The app shows:
    GPIO 0    -> real start button
    GPIO 2    -> real machine output
    Probe 200 -> machineRunning
    Probe 201 -> progressPercent
    Probe 202 -> completedCycles
*/

#include "esp32_ble_debugger_Lite.h"

const int START_BUTTON = 0;
const int MACHINE_PIN = 2;

volatile float machineRunning = 0;
volatile float progressPercent = 0;
volatile float completedCycles = 0;

bool lastButton = HIGH;
bool outputState = LOW;

unsigned long cycleStartedAt = 0;
unsigned long lastToggle = 0;

void setup() {
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(MACHINE_PIN, OUTPUT);

  ESP32_PROBE_GPIO(START_BUTTON, "DIGITAL", "IN");
  ESP32_PROBE_GPIO(MACHINE_PIN, "DIGITAL", "OUT");

  ESP32_PROBE_VIRTUAL(200, machineRunning);
  ESP32_PROBE_VIRTUAL(201, progressPercent);
  ESP32_PROBE_VIRTUAL(202, completedCycles);

  esp32_ble_debugger_begin(250, "Machine-Cycle");
}

void loop() {
  bool button = digitalRead(START_BUTTON);
  unsigned long now = millis();

  // Start a new cycle only when the machine is idle.
  if (lastButton == HIGH && button == LOW && machineRunning == 0) {
    machineRunning = 1;
    progressPercent = 0;
    cycleStartedAt = now;
    delay(50);
  }

  if (machineRunning == 1) {
    unsigned long elapsed = now - cycleStartedAt;

    progressPercent = elapsed * 100.0f / 10000.0f;

    // Toggle the output every 500 ms while running.
    if (now - lastToggle >= 500) {
      lastToggle = now;
      outputState = !outputState;
      digitalWrite(MACHINE_PIN, outputState);
    }

    // Finish after 10 seconds.
    if (elapsed >= 10000) {
      machineRunning = 0;
      progressPercent = 100;
      completedCycles++;

      outputState = LOW;
      digitalWrite(MACHINE_PIN, LOW);
    }
  }

  lastButton = button;
  esp32_ble_debugger_loop();
}
