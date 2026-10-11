#pragma once

#include "Config.h"

class DeskState {
public:
  int currentHeight() const { return currentHeight_; }
  bool updateHeight(int height, uint32_t now);
  static bool isValidHeight(int height);

private:
  int currentHeight_ = -1;
  uint32_t lastHeightAt_ = 0;
};
