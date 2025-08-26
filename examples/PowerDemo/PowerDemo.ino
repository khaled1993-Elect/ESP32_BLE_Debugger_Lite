/*
  PowerDemo Example
  -----------------
  This demonstration showcases the ESP32 BLE Debugger Lite in a
  power‑electronics context.  It toggles two digital pins in a
  complementary fashion (simulating gate signals for a half‑bridge)
  and blinks a status LED.  At the same time, it streams two
  slow virtual waveforms: an amplitude‑modulated sine and a triangle
  wave.  All signals are generated in software; no external inputs
  are required.  The companion app will automatically scale the
  virtual signals.  No sampling interval is specified here—your
  app sets it after connecting.

  Digital pins:
    • GPIO25 – high‑side gate (output)
    • GPIO26 – low‑side gate (complementary output)
    • GPIO27 – status LED (output)

  Virtual probes:
    • ID 110 – amplitude‑modulated sine wave
    • ID 111 – triangle wave
*/

#include "esp32_ble_debugger_Lite.h"
#include <math.h>

// Virtual signals (floating point; the app auto‑scales them)
volatile float vAM = 0.0f;
volatile float vTri = 0.0f;

void setup() {
  // Configure digital outputs
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);

  // Register digital probes
  ESP32_PROBE_GPIO(25, "DIGITAL", "OUT");
  ESP32_PROBE_GPIO(26, "DIGITAL", "OUT");
  ESP32_PROBE_GPIO(27, "DIGITAL", "OUT");

  // Register virtual probes (IDs should be ≥100 and unique)
  ESP32_PROBE_VIRTUAL(110, vAM);
  ESP32_PROBE_VIRTUAL(111, vTri);

  // Start BLE streaming.  The app will set the sampling interval.
  esp32_ble_debugger_begin();
}

void loop() {
  // Toggle the two gate outputs at 2 Hz (250 ms per state).  They are
  // complementary: when one is HIGH, the other is LOW.
  static uint32_t lastToggle = 0;
  static bool gateState = false;
  if (millis() - lastToggle >= 250) {
    lastToggle += 250;
    gateState = !gateState;
    digitalWrite(25, gateState);
    digitalWrite(26, !gateState);
  }

  // Blink the status LED at 1 Hz (on/off every 500 ms)
  static uint32_t lastBlink = 0;
  if (millis() - lastBlink >= 500) {
    lastBlink += 500;
    digitalWrite(27, !digitalRead(27));
  }

  // Generate an amplitude‑modulated sine wave: 1 Hz carrier, 0.2 Hz envelope
  float t = millis() * 0.001f;
  float carrier = sinf(2.0f * M_PI * 1.0f * t);
  float envelope = 1.0f + 0.5f * sinf(2.0f * M_PI * 0.2f * t);
  vAM = (carrier * envelope) / 1.5f; // roughly –1..+1

  // Generate a triangle wave at 0.5 Hz in the range −1..+1
  float period = 2.0f;  // seconds per cycle (0.5 Hz)
  float p = fmodf(t, period) / period; // fractional phase (0..1)
  if (p < 0.5f) {
    vTri = 4.0f * p - 1.0f;        // rising edge (–1 to +1)
  } else {
    vTri = -4.0f * p + 3.0f;       // falling edge (+1 to –1)
  }

  // Service the debugger loop (no‑op when ESP_LIVE_DBG_ASYNC==1)
  esp32_ble_debugger_loop();
}