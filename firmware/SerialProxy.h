#pragma once

#include <HardwareSerial.h>
#include "Frame.h"

class SerialProxy {
public:
  void begin();
  int readKeypad();
  int readController();
  void sendToController(uint8_t byte);
  void sendToController(const Frame& frame);

private:
  HardwareSerial keypad{1};
  HardwareSerial controller{0};
};
