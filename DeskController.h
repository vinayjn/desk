#pragma once

#include "SerialProxy.h"
#include "FrameBuilder.h"
#include "Logger.h"
#include "DeskState.h"

class DeskController {
public:
  void begin();
  void loop();

private:
  SerialProxy proxy;
  FrameBuilder keypadFrames{Config::KeypadHeader};
  FrameBuilder controllerFrames{Config::ControllerHeader};
  Logger logger;
  DeskState state;
  bool queryRequested_ = true;
  uint32_t startedAt_ = 0;

  void onControllerFrame(const Frame& frame, uint32_t now);
};
