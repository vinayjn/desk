#pragma once

#include "Frame.h"

class Logger {
public:
  void logFrame(const Frame& frame);

private:
  Frame last_[2];
  bool hasLast_[2] = {false, false};
};
