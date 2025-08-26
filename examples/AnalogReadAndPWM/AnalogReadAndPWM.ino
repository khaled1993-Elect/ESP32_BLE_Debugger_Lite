/*
  AnalogReadAndPWM Example
  ------------------------
  This sketch demonstrates how to read an analog voltage and expose it
  as a floating‑point virtual probe.  It also drives an LED using the
  ESP32's LEDC PWM hardware based on the analog input.  The ADC value is
  converted to volts (0.0–3.3 V) and streamed as probe ID 200.  No
  sampling interval is specified—your companion app will set it after
  connecting.
*/

#include "esp32_ble_debugger_Lite.h"

static const int ADC_PIN = 34;  // input‑only ADC on many ESP32 boards
static const int LED_PIN = 25;  // any LED‑capable pin

// Virtual probe for the analog voltage in volts (0..3.3)
volatile float vAnalogVolts = 0.0f;

void setup() {
  pinMode(ADC_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Setup LEDC PWM: channel 0, 1 kHz, 8‑bit resolution
  ledcSetup(0, 1000, 8);
  ledcAttachPin(LED_PIN, 0);

  // Register probes: monitor the LED as a digital output and stream
  // the analog voltage as a virtual probe with ID 200
  ESP32_PROBE_GPIO(LED_PIN, "DIGITAL", "OUT");
  ESP32_PROBE_VIRTUAL(200, vAnalogVolts);

  // Start the debugger.  Do not pass a sampling interval; the app sets it.
  esp32_ble_debugger_begin();
}

void loop() {
  // Read the raw ADC value (0..4095) and convert to volts
  int raw = analogRead(ADC_PIN);
  if (raw < 0) raw = 0;
  if (raw > 4095) raw = 4095;
  vAnalogVolts = (raw / 4095.0f) * 3.3f;

  // Map the raw ADC value to LED brightness (0..255) and write via LEDC
  uint8_t duty = (uint8_t)((raw * 255U) / 4095U);
  ledcWrite(0, duty);

  // Service the debugger loop (no‑op in async mode)
  esp32_ble_debugger_loop();
}