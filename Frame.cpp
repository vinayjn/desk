#include "Frame.h"

Frame::Source Frame::source() const {
  if (bytes_[0] == Config::KeypadHeader) return Source::Keypad;
  if (bytes_[0] == Config::ControllerHeader) return Source::Controller;
  return Source::Unknown;
}

bool Frame::isIdle() const {
  if (source() == Source::Keypad) return bytes_[2] == 0;
  return source() == Source::Controller &&
         bytes_[1] == 0 && bytes_[2] == 0 && bytes_[3] == 0;
}

const char* Frame::label() const {
  if (source() == Source::Keypad) return isIdle() ? "KEY:IDLE" : "KEY:UNKNOWN";
  if (source() == Source::Controller) return isIdle() ? "CTRL:IDLE" : "CTRL:UNKNOWN";
  return "UNKNOWN";
}
