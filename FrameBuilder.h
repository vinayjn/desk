#pragma once

#include "Frame.h"

class FrameBuilder {
public:
  explicit FrameBuilder(uint8_t header) : header_(header) {}

  bool addByte(uint8_t byte, uint32_t now, Frame& result);
  void expire(uint32_t now);
  bool isBuilding() const { return count_ != 0; }

private:
  uint8_t header_;
  Frame::Bytes bytes_{};
  size_t count_ = 0;
  uint32_t startedAt_ = 0;
};
