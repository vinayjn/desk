#pragma once

#include "SerialProxy.h"
#include "FrameBuilder.h"
#include "Logger.h"

class DeskController {
public:
  void begin();
  void loop();

private:
  SerialProxy proxy;
  FrameBuilder keypadFrames{Config::KeypadHeader};
  FrameBuilder controllerFrames{Config::ControllerHeader};
  Logger logger;
};
