#include <assert.h>
#include <stdio.h>
#include "DeskController.h"

uint32_t fakeNow = 0;
FakeSerial Serial;
FakeUart fakeUarts[2];
BLEServer fakeBleServer;
BLEAdvertising fakeAdvertising;

static void reset() {
  fakeNow = 0;
  Serial = FakeSerial();
  fakeUarts[0] = FakeUart();
  fakeUarts[1] = FakeUart();
}

static void feed(int port, const Frame& frame) {
  for (uint8_t byte : frame.bytes()) fakeUarts[port].input.push_back(byte);
}

static Frame height(int value) {
  const uint8_t digits[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  const uint8_t a = digits[value / 100];
  const uint8_t b = digits[(value / 10) % 10];
  const uint8_t c = digits[value % 10];
  return Frame(Frame::Bytes{{0x5A, a, b, c, uint8_t(a + b + c)}});
}

static std::vector<uint8_t> bytes(const Frame& frame) {
  return {frame.bytes().begin(), frame.bytes().end()};
}

static void startMove(DeskController& desk) {
  fakeNow = 100;
  desk.loop();
  desk.requestMove(100);
  desk.loop();
  feed(0, height(90));
  desk.loop();
  fakeUarts[0].output.clear();
  fakeUarts[1].output.clear();
}

static void testRelayAndLogging() {
  reset();
  DeskController desk;
  desk.begin();
  assert(fakeUarts[1].rx == 1 && fakeUarts[1].tx == 3);
  assert(fakeUarts[0].rx == 4 && fakeUarts[0].tx == 0);
  const std::vector<uint8_t> noise{0x12, 0x34, 0xFF, 0xA5, 0x5A};
  for (uint8_t byte : noise) fakeUarts[1].input.push_back(byte);
  feed(0, height(90));
  desk.loop();
  assert(fakeUarts[0].output == noise);
  assert(fakeUarts[1].output == bytes(height(90)));

  fakeNow = 101;
  desk.loop();
  Serial.output.clear();
  feed(1, Frame::upCommand()); desk.loop();
  feed(0, height(91)); desk.loop();
  feed(1, Frame::upCommand()); desk.loop();
  feed(1, Frame::idleCommand()); desk.loop();
  feed(1, Frame::upCommand()); desk.loop();
  const std::string needle = "KEY:UP ->";
  const size_t first = Serial.output.find(needle);
  const size_t second = Serial.output.find(needle, first + 1);
  assert(first != std::string::npos && second != std::string::npos);
  assert(Serial.output.find(needle, second + 1) == std::string::npos);
}

static void testInjectionBoundary() {
  reset();
  DeskController desk;
  desk.begin();
  fakeNow = 100;
  fakeUarts[1].input.push_back(0xA5);
  fakeUarts[1].input.push_back(0);
  desk.requestMove(100);
  desk.loop();
  assert(fakeUarts[0].output == (std::vector<uint8_t>{0xA5, 0}));
  for (size_t index = 2; index < 5; ++index) {
    fakeUarts[1].input.push_back(Frame::upCommand().bytes()[index]);
  }
  desk.loop();
  auto expected = bytes(Frame::upCommand());
  const auto query = bytes(Frame::queryCommand());
  expected.insert(expected.end(), query.begin(), query.end());
  assert(fakeUarts[0].output == expected);
}

static void testPhysicalCancellation() {
  reset();
  DeskController desk;
  desk.begin();
  startMove(desk);
  feed(1, Frame::downCommand());
  desk.loop();
  auto expected = bytes(Frame::idleCommand());
  const auto down = bytes(Frame::downCommand());
  expected.insert(expected.end(), down.begin(), down.end());
  assert(fakeUarts[0].output == expected);
  fakeUarts[0].output.clear();
  feed(1, Frame::idleCommand()); desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::idleCommand()));
}

static void testStopInsideBlockedPacket() {
  reset();
  DeskController desk;
  desk.begin();
  startMove(desk);
  fakeUarts[1].input.push_back(0xA5);
  fakeUarts[1].input.push_back(0);
  desk.loop();
  assert(fakeUarts[0].output.empty());
  feed(0, height(101));
  desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::idleCommand()));
  fakeUarts[0].output.clear();
  for (size_t index = 2; index < 5; ++index) {
    fakeUarts[1].input.push_back(Frame::idleCommand().bytes()[index]);
  }
  desk.loop();
  assert(fakeUarts[0].output.empty());
  feed(1, Frame::upCommand()); desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::upCommand()));
}

static void testSerialInput() {
  reset();
  DeskController desk;
  desk.begin();
  fakeNow = 100;
  desk.loop();
  fakeUarts[0].output.clear();
  Serial.enter("10000000000000000000\n100x\n");
  desk.loop();
  assert(fakeUarts[0].output.empty());
  Serial.enter("100\r\n");
  desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::queryCommand()));
  fakeUarts[0].output.clear();
  Serial.enter("s\n"); desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::idleCommand()));
}

static void testPendingTargetCancellation() {
  reset();
  DeskController desk;
  desk.begin();
  fakeNow = 100;
  desk.loop();
  fakeUarts[0].output.clear();
  desk.requestMove(100);
  feed(1, Frame::downCommand());
  desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::downCommand()));
  feed(0, height(90)); desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::downCommand()));
}

static void testBluetooth() {
  reset();
  DeskController desk;
  desk.begin();
  fakeNow = 100;
  desk.loop();
  auto* current = fakeBleServer.service.find(Config::BleHeightUuid);
  auto* target = fakeBleServer.service.find(Config::BleTargetUuid);
  auto* status = fakeBleServer.service.find(Config::BleStatusUuid);
  assert(current->getValue() == String(1, '\0'));
  assert(status->getValue() == "IDLE");

  fakeBleServer.connect();
  desk.loop();
  fakeUarts[0].output.clear();
  target->writeFromClient(String(1, char(100)));
  assert(fakeUarts[0].output.empty());
  assert(status->getValue() == "IDLE");
  desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::queryCommand()));
  assert(status->getValue() == "MOVING");
  fakeUarts[0].output.clear();
  feed(0, height(90)); desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::upCommand()));
  assert(current->getValue() == String(1, char(90)));

  fakeUarts[0].output.clear();
  target->writeFromClient("100");
  desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::idleCommand()));
  assert(status->getValue() == "ERROR");
  fakeUarts[0].output.clear();
  target->writeFromClient(String()); desk.loop();
  assert(fakeUarts[0].output.empty());
  assert(status->getValue() == "ERROR");

  target->writeFromClient(String(1, char(100))); desk.loop();
  feed(0, height(100)); desk.loop();
  assert(status->getValue() == "IDLE");
  assert(current->getValue() == String(1, char(100)));
  const unsigned heightNotifications = current->notifications;
  const unsigned statusNotifications = status->notifications;
  desk.loop();
  assert(current->notifications == heightNotifications);
  assert(status->notifications == statusNotifications);

  target->writeFromClient(String(1, char(90)));
  target->writeFromClient(String(1, char(100)));
  desk.loop();
  fakeUarts[0].output.clear();
  feed(0, height(95)); desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::upCommand()));

  fakeUarts[0].output.clear();
  target->writeFromClient(String(1, char(110)));
  fakeBleServer.disconnect();
  assert(fakeUarts[0].output.empty());
  desk.loop();
  assert(fakeUarts[0].output == bytes(Frame::idleCommand()));
  assert(status->getValue() == "IDLE");
  assert(fakeAdvertising.starts == 2);
}

int main() {
  testRelayAndLogging();
  testInjectionBoundary();
  testPhysicalCancellation();
  testStopInsideBlockedPacket();
  testSerialInput();
  testPendingTargetCancellation();
  testBluetooth();
  puts("Controller routing, logging, cancellation, USB and Bluetooth tests passed");
}
