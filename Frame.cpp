#include "Frame.h"

Frame::Source Frame::source() const {
  if (bytes_[0] == Config::KeypadHeader) return Source::Keypad;
  if (bytes_[0] == Config::ControllerHeader) return Source::Controller;
  return Source::Unknown;
}

bool Frame::isValid() const {
  if (source() == Source::Keypad) {
    return bytes_[1] == 0 && bytes_[3] == uint8_t(0xFF - bytes_[2]) && bytes_[4] == 0xFF;
  }
  if (source() == Source::Controller) {
    return bytes_[4] == uint8_t(bytes_[1] + bytes_[2] + bytes_[3]);
  }
  return false;
}

Frame::Kind Frame::kind() const {
  if (!isValid()) return Kind::Unknown;
  if (source() == Source::Controller) {
    if (bytes_[1] == 0 && bytes_[2] == 0 && bytes_[3] == 0) return Kind::Idle;
    if (segmentToDigit(bytes_[1]) >= 0 && segmentToDigit(bytes_[2]) >= 0 &&
        segmentToDigit(bytes_[3]) >= 0) return Kind::Height;
    return Kind::Unknown;
  }
  switch (bytes_[2]) {
    case 0x00: return Kind::Idle;
    case 0x01: return Kind::Query;
    case 0x02: return Kind::Preset1;
    case 0x04: return Kind::Preset2;
    case 0x08: return Kind::Preset3;
    case 0x10: return Kind::Preset4;
    case 0x20: return Kind::Up;
    case 0x40: return Kind::Down;
    default: return Kind::Unknown;
  }
}

bool Frame::isIdle() const { return kind() == Kind::Idle; }

int Frame::decodeHeight() const {
  if (kind() != Kind::Height) return -1;
  return 100 * segmentToDigit(bytes_[1]) + 10 * segmentToDigit(bytes_[2]) + segmentToDigit(bytes_[3]);
}

Frame Frame::keypadCommand(uint8_t command) {
  return Frame(Bytes{{Config::KeypadHeader, 0, command, uint8_t(0xFF - command), 0xFF}});
}

const char* Frame::label() const {
  if (!isValid()) {
    if (source() == Source::Keypad) return "KEY:INVALID";
    if (source() == Source::Controller) return "CTRL:INVALID";
    return "UNKNOWN";
  }
  switch (kind()) {
    case Kind::Idle: return source() == Source::Keypad ? "KEY:IDLE" : "CTRL:IDLE";
    case Kind::Up: return "KEY:UP";
    case Kind::Down: return "KEY:DOWN";
    case Kind::Preset1: return "KEY:PRESET1";
    case Kind::Preset2: return "KEY:PRESET2";
    case Kind::Preset3: return "KEY:PRESET3";
    case Kind::Preset4: return "KEY:PRESET4";
    case Kind::Query: return "KEY:QUERY";
    case Kind::Height: return "CTRL:HEIGHT";
    default: return source() == Source::Keypad ? "KEY:UNKNOWN" : "CTRL:UNKNOWN";
  }
}

int Frame::segmentToDigit(uint8_t segment) {
  switch (segment) {
    case 0x3F: return 0;
    case 0x06: return 1;
    case 0x5B: return 2;
    case 0x4F: return 3;
    case 0x66: return 4;
    case 0x6D: return 5;
    case 0x7D: return 6;
    case 0x07: return 7;
    case 0x7F: return 8;
    case 0x6F: return 9;
    default: return -1;
  }
}
