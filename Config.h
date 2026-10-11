#pragma once

#include <stdint.h>
#include <stddef.h>

namespace Config {
constexpr uint32_t DebugBaud = 115200;
constexpr uint32_t DeskBaud = 9600;

constexpr int KeypadRxPin = 1;
constexpr int KeypadTxPin = 3;
constexpr int ControllerRxPin = 4;
constexpr int ControllerTxPin = 0;
constexpr unsigned BytesPerLoop = 64;
constexpr size_t FrameSize = 5;
constexpr uint8_t KeypadHeader = 0xA5;
constexpr uint8_t ControllerHeader = 0x5A;
constexpr uint32_t FrameTimeoutMs = 100;
constexpr bool LogKeypadFrames = true;
constexpr bool LogControllerFrames = true;
constexpr bool LogIdleFrames = false;
}
