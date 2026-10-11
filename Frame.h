#pragma once

#include <array>
#include "Config.h"

class Frame {
public:
  using Bytes = std::array<uint8_t, Config::FrameSize>;
  enum class Source { Unknown, Keypad, Controller };
  enum class Kind { Unknown, Idle, Up, Down, Preset1, Preset2, Preset3, Preset4, Query, Height };

  Frame() = default;
  explicit Frame(const Bytes& bytes) : bytes_(bytes) {}

  const Bytes& bytes() const { return bytes_; }
  Source source() const;
  Kind kind() const;
  bool isValid() const;
  bool isIdle() const;
  int decodeHeight() const;
  const char* label() const;
  bool operator==(const Frame& other) const { return bytes_ == other.bytes_; }

private:
  Bytes bytes_{};
  static int segmentToDigit(uint8_t segment);
};
