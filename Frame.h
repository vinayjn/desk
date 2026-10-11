#pragma once

#include <array>
#include "Config.h"

class Frame {
public:
  using Bytes = std::array<uint8_t, Config::FrameSize>;
  enum class Source { Unknown, Keypad, Controller };

  Frame() = default;
  explicit Frame(const Bytes& bytes) : bytes_(bytes) {}

  const Bytes& bytes() const { return bytes_; }
  Source source() const;
  bool isIdle() const;
  const char* label() const;
  bool operator==(const Frame& other) const { return bytes_ == other.bytes_; }

private:
  Bytes bytes_{};
};
