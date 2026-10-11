#include "DeskState.h"

bool DeskState::isValidHeight(int height) {
  return height >= Config::MinHeight && height <= Config::MaxHeight;
}

bool DeskState::updateHeight(int height, uint32_t now) {
  if (!isValidHeight(height)) return false;
  const bool changed = height != currentHeight_;
  currentHeight_ = height;
  lastHeightAt_ = now;
  if (isMoving() && waitingForHeight_) {
    waitingForHeight_ = false;
    movingUp_ = currentHeight_ < targetHeight_;
    lastCommandAt_ = now - Config::MoveCommandIntervalMs;
  }
  return changed;
}

bool DeskState::startMove(int target, uint32_t now) {
  if (!isValidHeight(target)) {
    targetHeight_ = -1;
    status_ = Status::Error;
    return false;
  }
  targetHeight_ = target;
  status_ = Status::Moving;
  waitingForHeight_ = true;
  moveStartedAt_ = now;
  return true;
}

void DeskState::cancelMove() {
  status_ = Status::Idle;
  targetHeight_ = -1;
  waitingForHeight_ = false;
}

DeskState::Action DeskState::tick(uint32_t now) {
  if (!isMoving()) return Action::None;
  const bool timedOut = uint32_t(now - moveStartedAt_) >= Config::MoveTimeoutMs;
  const bool feedbackMissing = waitingForHeight_
    ? uint32_t(now - moveStartedAt_) >= Config::HeightTimeoutMs
    : uint32_t(now - lastHeightAt_) >= Config::HeightTimeoutMs;
  if (timedOut || feedbackMissing) {
    status_ = Status::Error;
    targetHeight_ = -1;
    return Action::Idle;
  }
  if (waitingForHeight_) return Action::None;

  if ((movingUp_ && currentHeight_ >= targetHeight_) ||
      (!movingUp_ && currentHeight_ <= targetHeight_)) {
    cancelMove();
    return Action::Idle;
  }
  if (uint32_t(now - lastCommandAt_) < Config::MoveCommandIntervalMs) return Action::None;
  lastCommandAt_ = now;
  return movingUp_ ? Action::Up : Action::Down;
}

const char* DeskState::statusName(Status status) {
  switch (status) {
    case Status::Idle: return "IDLE";
    case Status::Moving: return "MOVING";
    case Status::Error: return "ERROR";
  }
  return "ERROR";
}
