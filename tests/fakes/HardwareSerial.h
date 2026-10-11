#pragma once

#include "Arduino.h"
#include <vector>

struct FakeUart {
  std::deque<uint8_t> input;
  std::vector<uint8_t> output;
  int rx = -1;
  int tx = -1;
};

extern FakeUart fakeUarts[2];

class HardwareSerial {
public:
  explicit HardwareSerial(int port) : port_(port) {}
  void begin(uint32_t, int, int rx, int tx) {
    fakeUarts[port_].rx = rx;
    fakeUarts[port_].tx = tx;
  }
  int read() {
    auto& input = fakeUarts[port_].input;
    if (input.empty()) return -1;
    const uint8_t byte = input.front();
    input.pop_front();
    return byte;
  }
  void write(uint8_t byte) { fakeUarts[port_].output.push_back(byte); }
  void write(const uint8_t* bytes, size_t count) {
    auto& output = fakeUarts[port_].output;
    output.insert(output.end(), bytes, bytes + count);
  }

private:
  int port_;
};
