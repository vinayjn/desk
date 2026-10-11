#pragma once

#include "Config.h"

class DeskState {
public:
  enum class Status { Idle, Moving, Error };
  enum class Action { None, Up, Down, Idle };

  int currentHeight() const { return currentHeight_; }
  int targetHeight() const { return targetHeight_; }
  Status status() const { return status_; }
  bool isMoving() const { return status_ == Status::Moving; }
  bool updateHeight(int height, uint32_t now);
  bool startMove(int target, uint32_t now);
  void cancelMove();
  Action tick(uint32_t now);
  static bool isValidHeight(int height);
  static const char* statusName(Status status);

private:
  int currentHeight_ = -1;
  uint32_t lastHeightAt_ = 0;
  int targetHeight_ = -1;
  Status status_ = Status::Idle;
  bool waitingForHeight_ = false;
  bool movingUp_ = false;
  uint32_t moveStartedAt_ = 0;
  uint32_t lastCommandAt_ = 0;
};
