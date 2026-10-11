#include "DeskState.h"

bool DeskState::isValidHeight(int height) {
  return height >= Config::MinHeight && height <= Config::MaxHeight;
}

bool DeskState::updateHeight(int height, uint32_t now) {
  if (!isValidHeight(height)) return false;
  const bool changed = height != currentHeight_;
  currentHeight_ = height;
  lastHeightAt_ = now;
  return changed;
}
