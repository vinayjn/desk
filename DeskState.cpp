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
    progressHeight_ = currentHeight_;
    lastProgressAt_ = now;
  } else if (isMoving() &&
      ((movingUp_ && currentHeight_ > progressHeight_) ||
       (!movingUp_ && currentHeight_ < progressHeight_))) {
    // Repeated reports refresh feedback, but only progress toward the target
    // refreshes this timer. Oscillating heights must not hide a stall.
    progressHeight_ = currentHeight_;
    lastProgressAt_ = now;
  }
  return changed;
}

bool DeskState::startMove(int target, uint32_t now) {
  if (!isValidHeight(target)) {
    failMove(ErrorReason::InvalidTarget);
    return false;
  }
  targetHeight_ = target;
  status_ = Status::Moving;
  errorReason_ = ErrorReason::None;
  waitingForHeight_ = true;
  moveStartedAt_ = now;
  return true;
}

void DeskState::cancelMove() {
  status_ = Status::Idle;
  errorReason_ = ErrorReason::None;
  targetHeight_ = -1;
  waitingForHeight_ = false;
}

DeskState::Action DeskState::tick(uint32_t now) {
  if (!isMoving()) return Action::None;
  const bool feedbackMissing = waitingForHeight_
    ? uint32_t(now - moveStartedAt_) >= Config::HeightTimeoutMs
    : uint32_t(now - lastHeightAt_) >= Config::HeightTimeoutMs;
  if (feedbackMissing) return failMove(ErrorReason::FeedbackTimeout);
  if (waitingForHeight_) return Action::None;

  if ((movingUp_ && currentHeight_ >= targetHeight_) ||
      (!movingUp_ && currentHeight_ <= targetHeight_)) {
    cancelMove();
    return Action::Idle;
  }
  if (uint32_t(now - moveStartedAt_) >= Config::MoveTimeoutMs) {
    return failMove(ErrorReason::MoveTimeout);
  }
  if (uint32_t(now - lastProgressAt_) >= Config::MoveProgressTimeoutMs) {
    return failMove(ErrorReason::NoProgress);
  }
  if (uint32_t(now - lastCommandAt_) < Config::MoveCommandIntervalMs) return Action::None;
  lastCommandAt_ = now;
  return movingUp_ ? Action::Up : Action::Down;
}

DeskState::Action DeskState::failMove(ErrorReason reason) {
  status_ = Status::Error;
  errorReason_ = reason;
  targetHeight_ = -1;
  waitingForHeight_ = false;
  return Action::Idle;
}

const char* DeskState::statusName(Status status) {
  switch (status) {
    case Status::Idle: return "IDLE";
    case Status::Moving: return "MOVING";
    case Status::Error: return "ERROR";
  }
  return "ERROR";
}

const char* DeskState::errorReasonName(ErrorReason reason) {
  switch (reason) {
    case ErrorReason::None: return "none";
    case ErrorReason::InvalidTarget: return "invalid target";
    case ErrorReason::MoveTimeout: return "movement deadline exceeded";
    case ErrorReason::FeedbackTimeout: return "height feedback timed out";
    case ErrorReason::NoProgress: return "no progress toward target";
  }
  return "unknown";
}
