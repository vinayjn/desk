#pragma once

#include "Config.h"

class DeskState {
public:
  enum class Status { Idle, Moving, Error };
  enum class Action { None, Up, Down, Idle };
  enum class ErrorReason { None, InvalidTarget, MoveTimeout, FeedbackTimeout, NoProgress };

  int currentHeight() const { return currentHeight_; }
  int targetHeight() const { return targetHeight_; }
  Status status() const { return status_; }
  ErrorReason errorReason() const { return errorReason_; }
  bool isMoving() const { return status_ == Status::Moving; }
  bool isWaitingForHeight() const { return isMoving() && waitingForHeight_; }
  bool updateHeight(int height, uint32_t now);
  bool startMove(int target, uint32_t now);
  void cancelMove();
  Action tick(uint32_t now);
  static bool isValidHeight(int height);
  static const char* statusName(Status status);
  static const char* errorReasonName(ErrorReason reason);

private:
  int currentHeight_ = -1;
  uint32_t lastHeightAt_ = 0;
  int targetHeight_ = -1;
  Status status_ = Status::Idle;
  ErrorReason errorReason_ = ErrorReason::None;
  bool waitingForHeight_ = false;
  bool movingUp_ = false;
  uint32_t moveStartedAt_ = 0;
  uint32_t lastCommandAt_ = 0;
  uint32_t lastProgressAt_ = 0;
  int progressHeight_ = -1;

  Action failMove(ErrorReason reason);
};
