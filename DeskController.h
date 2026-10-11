#pragma once

#include "SerialProxy.h"
#include "FrameBuilder.h"
#include "Logger.h"
#include "DeskState.h"

class DeskController {
public:
  void begin();
  void loop();
  void requestMove(int target);
  void stopMove();

private:
  SerialProxy proxy;
  FrameBuilder keypadFrames{Config::KeypadHeader};
  FrameBuilder controllerFrames{Config::ControllerHeader};
  Logger logger;
  DeskState state;
  bool queryRequested_ = true;
  uint32_t startedAt_ = 0;
  bool moveRequested_ = false;
  int requestedTarget_ = 0;
  bool keypadPacketBlocked_ = false;
  char serialCommand_[16]{};
  size_t serialCommandLength_ = 0;
  bool serialCommandOverflow_ = false;
  DeskState::Status lastReportedStatus_ = DeskState::Status::Idle;

  void onKeypadFrame(const Frame& frame);
  void onControllerFrame(const Frame& frame, uint32_t now);
  void readSerialCommands();
  void updateMovement(uint32_t now);
};
