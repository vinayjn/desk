#pragma once

#include <stdint.h>

namespace Config {
constexpr uint32_t DebugBaud = 115200;
constexpr uint32_t DeskBaud = 9600;

constexpr int KeypadRxPin = 1;
constexpr int KeypadTxPin = 3;
constexpr int ControllerRxPin = 4;
constexpr int ControllerTxPin = 0;
constexpr unsigned BytesPerLoop = 64;
}
