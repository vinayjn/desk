#include "DeskController.h"
#include "Config.h"
#include <stdlib.h>
#include <string.h>

void DeskController::begin() {
  Serial.begin(Config::DebugBaud);
  proxy.begin();
  startedAt_ = millis();
  ble.begin();
  Serial.println("Desk ready: enter height in cm, q to query, or s to stop (newline)");
}

void DeskController::loop() {
  ble.poll();
  if (ble.takeStopRequest()) stopMove();
  if (ble.takeQueryRequest()) queryRequested_ = true;
  const int bleTarget = ble.takeTarget();
  if (bleTarget >= 0) requestMove(bleTarget);
  const uint32_t now = millis();
  keypadFrames.expire(now);
  controllerFrames.expire(now);
  if (!keypadFrames.isBuilding()) keypadPacketBlocked_ = false;
  Frame frame;

  for (unsigned count = 0; count < Config::BytesPerLoop; ++count) {
    int byte = proxy.readKeypad();
    if (byte < 0) break;
    if (!keypadFrames.isBuilding()) keypadPacketBlocked_ = state.isMoving();
    if (!keypadPacketBlocked_) proxy.sendToController(static_cast<uint8_t>(byte));
    if (keypadFrames.addByte(static_cast<uint8_t>(byte), millis(), frame)) {
      logger.logFrame(frame);
      onKeypadFrame(frame);
      if (!keypadFrames.isBuilding()) keypadPacketBlocked_ = false;
    }
  }
  for (unsigned count = 0; count < Config::BytesPerLoop; ++count) {
    int byte = proxy.readController();
    if (byte < 0) break;
    proxy.sendToKeypad(static_cast<uint8_t>(byte));
    if (controllerFrames.addByte(static_cast<uint8_t>(byte), millis(), frame)) {
      logger.logFrame(frame);
      onControllerFrame(frame, millis());
    }
  }
  readSerialCommands();
  if (moveRequested_ && !keypadFrames.isBuilding() &&
      uint32_t(millis() - startedAt_) >= Config::StartupQueryDelayMs) {
    const bool wasMoving = state.isMoving();
    if (state.startMove(requestedTarget_, millis())) {
      proxy.sendToController(Frame::queryCommand());
      queryRequested_ = false;
    } else if (wasMoving) {
      proxy.sendToController(Frame::idleCommand());
    }
    moveRequested_ = false;
  }
  if (queryRequested_ && !keypadFrames.isBuilding() &&
      uint32_t(millis() - startedAt_) >= Config::StartupQueryDelayMs) {
    proxy.sendToController(Frame::queryCommand());
    queryRequested_ = false;
  }
  updateMovement(millis());
  if (state.status() != lastReportedStatus_) {
    lastReportedStatus_ = state.status();
    if (Serial) Serial.printf("Status: %s\n", DeskState::statusName(state.status()));
  }
  ble.publish(state.currentHeight(), state.status());
}

void DeskController::onKeypadFrame(const Frame& frame) {
  if (!frame.isValid() || frame.isIdle() ||
      frame.kind() == Frame::Kind::Query) return;
  // The physical keypad wins. Replay its whole packet after stopping the
  // automatic move, including a packet that began just before a timeout.
  if (keypadPacketBlocked_) {
    stopMove();
    proxy.sendToController(frame);
  } else if (moveRequested_) {
    // This packet has already passed through. It also cancels a remote
    // request that was waiting for the current keypad packet to finish.
    moveRequested_ = false;
  }
}

void DeskController::onControllerFrame(const Frame& frame, uint32_t now) {
  if (frame.kind() != Frame::Kind::Height) return;
  if (state.updateHeight(frame.decodeHeight(), now) && Serial) {
    Serial.printf("Height: %d cm\n", state.currentHeight());
  }
}

void DeskController::requestMove(int target) {
  requestedTarget_ = target;
  moveRequested_ = true;
}

void DeskController::stopMove() {
  const bool wasMoving = state.isMoving();
  state.cancelMove();
  moveRequested_ = false;
  if (wasMoving) proxy.sendToController(Frame::idleCommand());
}

void DeskController::updateMovement(uint32_t now) {
  switch (state.tick(now)) {
    case DeskState::Action::Up: proxy.sendToController(Frame::upCommand()); break;
    case DeskState::Action::Down: proxy.sendToController(Frame::downCommand()); break;
    case DeskState::Action::Idle: proxy.sendToController(Frame::idleCommand()); break;
    case DeskState::Action::None: break;
  }
}

void DeskController::readSerialCommands() {
  for (unsigned count = 0; count < Config::BytesPerLoop && Serial.available(); ++count) {
    const char byte = static_cast<char>(Serial.read());
    if (byte == '\r') continue;
    if (byte != '\n') {
      if (serialCommandLength_ < sizeof(serialCommand_) - 1) {
        serialCommand_[serialCommandLength_++] = byte;
      } else {
        serialCommandOverflow_ = true;
      }
      continue;
    }
    serialCommand_[serialCommandLength_] = '\0';
    if (!serialCommandOverflow_ && serialCommandLength_ != 0) {
      if (strcmp(serialCommand_, "q") == 0) queryRequested_ = true;
      else if (strcmp(serialCommand_, "s") == 0) stopMove();
      else {
        char* end = nullptr;
        const long height = strtol(serialCommand_, &end, 10);
        if (*end == '\0' && height >= Config::MinHeight && height <= Config::MaxHeight) {
          requestMove(static_cast<int>(height));
        } else if (Serial) {
          Serial.printf("Enter a height from %d to %d cm, q, or s\n", Config::MinHeight, Config::MaxHeight);
        }
      }
    } else if (serialCommandOverflow_ && Serial) {
      Serial.println("Command too long; discarded");
    }
    serialCommandLength_ = 0;
    serialCommandOverflow_ = false;
  }
}
