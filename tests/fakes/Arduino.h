#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <deque>
#include <string>

#define CONFIG_IDF_TARGET_ESP32C3 1
#define ARDUINO_USB_CDC_ON_BOOT 1
#define SERIAL_8N1 0

using String = std::string;

extern uint32_t fakeNow;
inline uint32_t millis() { return fakeNow; }

struct FakeSerial {
  std::deque<uint8_t> input;
  std::string output;
  bool connected = true;
  void begin(uint32_t) {}
  explicit operator bool() const { return connected; }
  int available() const { return static_cast<int>(input.size()); }
  int read() {
    if (input.empty()) return -1;
    const uint8_t byte = input.front();
    input.pop_front();
    return byte;
  }
  void println(const char* text = "") { output += std::string(text) + "\n"; }
  void printf(const char* format, ...) {
    char buffer[256];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    output += buffer;
  }
  void enter(const char* text) { while (*text) input.push_back(*text++); }
};

extern FakeSerial Serial;
