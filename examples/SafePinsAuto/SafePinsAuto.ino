/*
  SafePinsAuto Example
  --------------------
  This sketch shows how to register all "safe" GPIOs automatically
  using the ESP32 BLE Debugger Lite.  The library maintains a list of
  pins that are generally safe to probe on the ESP32 (i.e. not used for
  bootstrapping or other special functions).  Calling registerSafePins()
  will register each safe pin as either a digital input or output based
  on its current configuration.  After starting the debugger, connect
  your companion app to view the live state of all safe pins.

  There is no need to specify a sampling interval; the app will
  automatically set one after connecting.
*/

#include "esp32_ble_debugger_Lite.h"

void setup() {
  // Register all safe GPIOs for live monitoring
  registerSafePins();

  // Start BLE streaming.  Do not specify a sampling interval; the app sets it.
  esp32_ble_debugger_begin();
}

void loop() {
  // In synchronous mode, call the debugger loop periodically.  In
  // asynchronous mode (default), this call does nothing.
  esp32_ble_debugger_loop();
}