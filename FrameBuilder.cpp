#include "FrameBuilder.h"

bool FrameBuilder::addByte(uint8_t byte, uint32_t now, Frame& result) {
  expire(now);
  if (count_ == 0) {
    if (byte != header_) return false;
    startedAt_ = now;
  }

  // A header-valued payload byte is data, not a new packet boundary.
  bytes_[count_++] = byte;
  if (count_ < Config::FrameSize) return false;
  result = Frame(bytes_);
  count_ = 0;
  return true;
}

void FrameBuilder::expire(uint32_t now) {
  if (count_ != 0 && uint32_t(now - startedAt_) >= Config::FrameTimeoutMs) {
    count_ = 0;
  }
}
