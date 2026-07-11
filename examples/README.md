# ESP32 BLE Debugger Lite - Beginner Examples

These examples are designed for an ESP32-S3 board and require no external
sensors.

They use:

- GPIO 0: onboard BOOT button
- GPIO 2: real digital output
- Virtual probes: internal program variables

## Examples

1. **Button Controls a Lamp**
   Press the button to toggle an output and count presses.

2. **Timed Stair Light**
   Press the button to turn the output on for five seconds.

3. **Door Open Alarm**
   Hold the button for three seconds to activate an alarm.

4. **Temperature Alarm**
   Test alarm logic using a simulated temperature.

5. **Machine Cycle Monitor**
   Start a timed machine cycle and observe progress.

## Important

Pressing BOOT while the sketch is running is safe.

Do not hold BOOT while resetting the board, because that places the ESP32-S3
in download mode.
