#include "Logger.h"
#include <Arduino.h>

void Logger::logFrame(const Frame& frame) {
  if (frame.source() == Frame::Source::Unknown) return;
  const size_t index = frame.source() == Frame::Source::Keypad ? 0 : 1;
  const bool enabled = index == 0 ? Config::LogKeypadFrames : Config::LogControllerFrames;
  if (!enabled || !Serial) return;

  if (frame.isIdle()) {

    hasLast_[index] = false;
    if (!Config::LogIdleFrames) return;
  } else if (hasLast_[index] && frame == last_[index]) {
    return;
  }

  Serial.printf("%s ->", frame.label());
  for (uint8_t byte : frame.bytes()) Serial.printf(" %02X", byte);
  Serial.println();
  last_[index] = frame;
  hasLast_[index] = true;
}
