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

static void testMovement() {
  using Action = DeskState::Action;
  using Status = DeskState::Status;
  DeskState state;
  assert(state.startMove(100, 0));
  assert(state.tick(0) == Action::None);
  state.updateHeight(90, 10);
  assert(state.tick(10) == Action::Up);
  assert(state.tick(109) == Action::None);
  assert(state.tick(110) == Action::Up);
  state.updateHeight(101, 120);
  assert(state.tick(120) == Action::Idle);
  assert(state.status() == Status::Idle);
  assert(state.tick(130) == Action::None);

  assert(state.startMove(90, 200));
  assert(state.tick(200) == Action::None);
  state.updateHeight(100, 201);
  assert(state.tick(201) == Action::Down);
  state.updateHeight(89, 220);
  assert(state.tick(220) == Action::Idle);

  assert(state.startMove(90, 300));
  state.updateHeight(90, 301);
  assert(state.tick(301) == Action::Idle);
  assert(!state.startMove(119, 400));
  assert(state.status() == Status::Error);
  assert(state.errorReason() == DeskState::ErrorReason::InvalidTarget);
  assert(state.tick(400) == Action::None);

  assert(state.startMove(100, 500));
  assert(state.tick(1499) == Action::None);
  assert(state.tick(1500) == Action::Idle);
  assert(state.status() == Status::Error);
  assert(state.errorReason() == DeskState::ErrorReason::FeedbackTimeout);

  assert(state.startMove(100, 2000));
  state.updateHeight(90, 2001);
  assert(state.tick(2001) == Action::Up);
  assert(state.tick(3001) == Action::Idle);
  assert(state.status() == Status::Error);
  assert(state.errorReason() == DeskState::ErrorReason::FeedbackTimeout);

  assert(state.startMove(100, 4000));
  for (uint32_t now = 4001; now < 14001; now += 100) {
    state.updateHeight(90, now);
    assert(state.tick(now) == Action::Up);
  }
  state.updateHeight(90, 14001);
  assert(state.tick(14001) == Action::Idle);
  assert(state.status() == Status::Error);
  assert(state.errorReason() == DeskState::ErrorReason::NoProgress);

  assert(state.startMove(100, UINT32_MAX - 50));
  state.updateHeight(90, UINT32_MAX - 40);
  assert(state.tick(UINT32_MAX - 40) == Action::Up);
  assert(state.tick(60) == Action::Up);
  state.cancelMove();
  assert(state.status() == Status::Idle);
  assert(state.errorReason() == DeskState::ErrorReason::None);
  assert(state.tick(70) == Action::None);
}

static void testLongMovement() {
  using Action = DeskState::Action;
  using Status = DeskState::Status;

  const int starts[] = {103, 75, 118, 72};
  const int targets[] = {75, 103, 72, 118};
  for (unsigned move = 0; move < 4; ++move) {
    DeskState state;
    const int direction = targets[move] > starts[move] ? 1 : -1;
    const Action action = direction > 0 ? Action::Up : Action::Down;
    assert(state.startMove(targets[move], 0));
    for (uint32_t now = 100; ; now += 100) {
      const int value = starts[move] + direction * int((now - 100) / 600);
      state.updateHeight(value, now);
      if (value == targets[move]) {
        assert(now > 10000);
        assert(state.tick(now) == Action::Idle);
        assert(state.status() == Status::Idle);
        break;
      }
      assert(state.tick(now) == action);
      assert(state.status() == Status::Moving);
    }
  }
}

static void testMovementLimits() {
  using Action = DeskState::Action;
  using Status = DeskState::Status;
  using Error = DeskState::ErrorReason;
  DeskState state;
  assert(state.startMove(75, 0));
  state.updateHeight(103, 100);
  assert(state.tick(100) == Action::Down);
  state.updateHeight(102, 5000);
  assert(state.tick(5000) == Action::Down);
  for (uint32_t now = 5100; now < 15000; now += 100) {
    state.updateHeight(now % 200 == 0 ? 102 : 103, now);
    state.tick(now);
    assert(state.status() == Status::Moving);
  }
  state.updateHeight(102, 15000);
  assert(state.tick(15000) == Action::Idle);
  assert(state.errorReason() == Error::NoProgress);
  assert(state.startMove(100, 16000));
  assert(state.errorReason() == Error::None);
  state.updateHeight(100, 16001);
  assert(state.tick(16001) == Action::Idle);

  const uint32_t starts[] = {0, UINT32_MAX - 1000};
  for (uint32_t started : starts) {
    DeskState slow;
    assert(slow.startMove(118, started));
    for (uint32_t elapsed = 100; elapsed < 60000; elapsed += 100) {
      const uint32_t now = started + elapsed;
      slow.updateHeight(72 + int(elapsed / 2000), now);
      assert(slow.tick(now) == Action::Up);
    }
    slow.updateHeight(102, started + 60000);
    assert(slow.tick(started + 60000) == Action::Idle);
    assert(slow.errorReason() == Error::MoveTimeout);
  }

  DeskState boundary;
  assert(boundary.startMove(118, 0));
  for (uint32_t elapsed = 100; elapsed < 60000; elapsed += 100) {
    boundary.updateHeight(72 + int(elapsed * 46 / 60000), elapsed);
    assert(boundary.tick(elapsed) == Action::Up);
  }
  boundary.updateHeight(118, 60000);
  assert(boundary.tick(60000) == Action::Idle);
  assert(boundary.status() == Status::Idle);
}

int main() {
  testFrames();
  testStreamParsing();
  testProtocol();
  testHeightState();
  testMovement();
  testLongMovement();
  testMovementLimits();
  puts("Frame, stream, protocol, height and movement tests passed");
}
