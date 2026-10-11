#include <assert.h>
#include <stdio.h>
#include "FrameBuilder.h"

static void testFrames() {
  const Frame up(Frame::Bytes{{0xA5, 0, 0x20, 0xDF, 0xFF}});
  const Frame idle(Frame::Bytes{{0xA5, 0, 0, 0xFF, 0xFF}});
  assert(up.source() == Frame::Source::Keypad);
  assert(!up.isIdle());
  assert(idle.isIdle());
  assert(up == Frame(up.bytes()));
  assert(!(up == idle));
}

static void testStreamParsing() {
  FrameBuilder parser(Config::KeypadHeader);
  Frame result;
  assert(!parser.addByte(0x12, 0, result));
  assert(!parser.isBuilding());
  const Frame::Bytes bytes{{0xA5, 0, 0x20, 0xDF, 0xFF}};
  for (size_t index = 0; index < bytes.size(); ++index) {
    assert(parser.addByte(bytes[index], index, result) == (index == 4));
  }
  assert(result.bytes() == bytes);
  assert(!parser.isBuilding());
  for (uint8_t byte : bytes) parser.addByte(byte, 10, result);
  assert(result.bytes() == bytes);

  const Frame::Bytes headerInPayload{{0xA5, 0xA5, 0, 0xFF, 0xFF}};
  for (uint8_t byte : headerInPayload) parser.addByte(byte, 20, result);
  assert(result.bytes() == headerInPayload);

  parser.addByte(0xA5, 30, result);
  parser.addByte(0, 31, result);
  assert(!parser.addByte(0x20, 130, result));
  assert(!parser.isBuilding());

  parser.addByte(0xA5, UINT32_MAX - 20, result);
  parser.expire(10);
  assert(parser.isBuilding());
  parser.expire(100);
  assert(!parser.isBuilding());
}

int main() {
  testFrames();
  testStreamParsing();
  puts("Frame and stream tests passed");
}
