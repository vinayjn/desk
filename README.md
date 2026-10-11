# Standing desk

ESP32-C3 Super Mini firmware that connects a standing desk keypad to its motor
controller. It relays keypad traffic, reads the current height, and accepts
height targets over Bluetooth or USB.

Automatic moves wait for fresh height feedback, then stop at the target.
Pressing a physical keypad button or disconnecting Bluetooth cancels the move.
Feedback, progress, and travel timeouts stop an unresponsive move. Heights and
timing limits are configured in `Config.h`.

## Wiring

Both UART links use 9600 baud, 8N1. RX/TX labels refer to the ESP32.

| Connection | ESP32 GPIO |
| --- | --- |
| Keypad TX → ESP32 RX | 1 |
| ESP32 TX → keypad RX | 3 |
| Controller TX → ESP32 RX | 4 |
| ESP32 TX → controller RX | 0 |

The ESP32 shares the desk cable's VCC and ground. Debug output uses native USB.

## Build

Use Arduino ESP32 core **3.3.12**, board **ESP32C3 Dev Module**, with
**USB CDC On Boot** enabled. No additional libraries are required.

```sh
bash scripts/test.sh
bash scripts/build.sh
```

Upload through Arduino IDE. The build script supports `ARDUINO_CLI`, `FQBN`,
and `DESK_BUILD_DIR` overrides.

## Control

Desky connects to the Bluetooth device **Standing Desk**. The service publishes
height as one binary byte in centimeters and status as `IDLE`, `MOVING`, or
`ERROR`; targets are one binary byte from 72 to 118. UUIDs are in `Config.h`.

For USB control, open the serial monitor at 115200 baud with newline endings.
Send a height such as `100`, `q` to query height, or `s` to stop.
