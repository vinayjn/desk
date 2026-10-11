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
  if (!result.isValid()) {
    // A truncated packet may have swallowed the next header. Keep that
    // suffix so the observer can recover without disturbing the relay.
    for (size_t start = 1; start < Config::FrameSize; ++start) {
      if (bytes_[start] != header_) continue;
      count_ = Config::FrameSize - start;
      for (size_t index = 0; index < count_; ++index) bytes_[index] = bytes_[start + index];
      startedAt_ = now;
      break;
    }
  }
  return true;
}

void FrameBuilder::expire(uint32_t now) {
  if (count_ != 0 && uint32_t(now - startedAt_) >= Config::FrameTimeoutMs) {
    count_ = 0;
  }
}
