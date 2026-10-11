#pragma once

#include <stdint.h>
#include <stddef.h>

namespace Config {
constexpr uint32_t DebugBaud = 115200;
constexpr uint32_t DeskBaud = 9600;

// RX/TX are named from the ESP32's perspective.
constexpr int KeypadRxPin = 1;
constexpr int ControllerRxPin = 6;
constexpr int ControllerTxPin = 7;
constexpr unsigned BytesPerLoop = 64;
constexpr size_t FrameSize = 5;
constexpr uint8_t KeypadHeader = 0xA5;
constexpr uint8_t ControllerHeader = 0x5A;
constexpr uint32_t FrameTimeoutMs = 100;
constexpr bool LogKeypadFrames = true;
constexpr bool LogControllerFrames = true;
constexpr bool LogIdleFrames = false;
constexpr int MinHeight = 72;
constexpr int MaxHeight = 118;
constexpr uint32_t StartupQueryDelayMs = 100;
constexpr uint32_t MoveCommandIntervalMs = 100;

constexpr uint32_t MoveTimeoutMs = 60000;
constexpr uint32_t MoveProgressTimeoutMs = 10000;

constexpr uint32_t InitialHeightTimeoutMs = 2500;
constexpr uint32_t HeightQueryRetryIntervalMs = 500;
constexpr uint32_t HeightTimeoutMs = 1000;
constexpr const char* BleDeviceName = "Standing Desk";
constexpr const char* BleServiceUuid = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
constexpr const char* BleHeightUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
constexpr const char* BleTargetUuid = "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e";
constexpr const char* BleStatusUuid = "9a8ca5fa-7d85-4e8e-9e3a-6c8d3f2b1e4d";
}
