#pragma once
#include "BLEDevice.h"

class BLE2902 : public BLEDescriptor {
public:
  BLE2902() : BLEDescriptor(BLEUUID(0x2902)) {}
};
