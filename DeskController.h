#pragma once

#include "SerialProxy.h"

class DeskController {
public:
  void begin();
  void loop();

private:
  SerialProxy proxy;
};
