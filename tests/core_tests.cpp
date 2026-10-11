#include <assert.h>
#include <stdio.h>
#include "FrameBuilder.h"
#include "DeskState.h"

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

  const Frame::Bytes headerInPayload{{0xA5, 0, 0x5A, 0xA5, 0xFF}};
  for (uint8_t byte : headerInPayload) parser.addByte(byte, 20, result);
  assert(result.bytes() == headerInPayload);
  assert(result.isValid());

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

static void testProtocol() {
  using Kind = Frame::Kind;
  const uint8_t commands[] = {0, 1, 2, 4, 8, 0x10, 0x20, 0x40};
  const Kind kinds[] = {Kind::Idle, Kind::Query, Kind::Preset1, Kind::Preset2,
                        Kind::Preset3, Kind::Preset4, Kind::Up, Kind::Down};
  for (size_t index = 0; index < sizeof(commands); ++index) {
    Frame::Bytes bytes{{0xA5, 0, commands[index], uint8_t(0xFF - commands[index]), 0xFF}};
    assert(Frame(bytes).isValid());
    assert(Frame(bytes).kind() == kinds[index]);
    bytes[3] ^= 1;
    assert(!Frame(bytes).isValid());
    assert(Frame(bytes).kind() == Kind::Unknown);
    bytes[3] ^= 1;
    bytes[4] = 0;
    assert(!Frame(bytes).isValid());
  }
  const Frame height(Frame::Bytes{{0x5A, 0x06, 0x06, 0x6D, 0x79}});
  assert(height.kind() == Kind::Height);
  assert(height.decodeHeight() == 115);
  assert(Frame(Frame::Bytes{{0x5A, 0, 0, 0, 0}}).isIdle());
  assert(Frame(Frame::Bytes{{0x5A, 0x06, 0x06, 0x6D, 0x78}}).decodeHeight() == -1);
  assert(Frame(Frame::Bytes{{0x5A, 0x06, 0x06, 0, 0x0C}}).decodeHeight() == -1);
  assert(Frame().decodeHeight() == -1);

  FrameBuilder parser(0xA5);
  Frame result;
  const uint8_t stream[] = {0xA5, 0, 0x20, 0xA5, 0, 0x40, 0xBF, 0xFF};
  unsigned validFrames = 0;
  for (uint8_t byte : stream) {
    if (parser.addByte(byte, 0, result) && result.isValid()) ++validFrames;
  }
  assert(validFrames == 1);
  assert(result.kind() == Kind::Down);
}

static void testHeightState() {
  DeskState state;
  assert(state.currentHeight() == -1);
  assert(!state.updateHeight(0, 0));
  assert(state.currentHeight() == -1);
  assert(state.updateHeight(72, 10));
  assert(!state.updateHeight(72, 20));
  assert(!state.updateHeight(119, 30));
  assert(state.currentHeight() == 72);
  assert(state.updateHeight(118, 40));
  const Frame query = Frame::queryCommand();
  assert(query.bytes() == (Frame::Bytes{{0xA5, 0, 1, 0xFE, 0xFF}}));
  assert(query.isValid());
  assert(query.kind() == Frame::Kind::Query);
}

int main() {
  testFrames();
  testStreamParsing();
  testProtocol();
  testHeightState();
  puts("Frame, stream, protocol and height tests passed");
}
