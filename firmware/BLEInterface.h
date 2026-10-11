#pragma once

#include <atomic>
#include <BLEDevice.h>
#include <BLEServer.h>
#include "DeskState.h"

// Callbacks run on the Bluetooth task. They only exchange small messages;
// DeskController remains the sole owner of state and serial command writes.
class BLEInterface : private BLEServerCallbacks, private BLECharacteristicCallbacks {
public:
  void begin();
  void poll();
  int takeTarget() {
    const int target = pendingTarget_.exchange(-1);
    return connected_.load() ? target : -1;
  }
  bool takeQueryRequest() { return queryRequested_.exchange(false); }
  bool takeStopRequest() { return stopRequested_.exchange(false); }
  void publish(int height, DeskState::Status status);

private:
  BLECharacteristic* height_ = nullptr;
  BLECharacteristic* target_ = nullptr;
  BLECharacteristic* status_ = nullptr;
  std::atomic<int> pendingTarget_{-1};
  std::atomic<bool> connected_{false};
  std::atomic<bool> queryRequested_{false};
  std::atomic<bool> stopRequested_{false};
  std::atomic<bool> restartAdvertising_{false};
  std::atomic<bool> publishRequested_{true};
  int lastHeight_ = -1;
  DeskState::Status lastStatus_ = DeskState::Status::Idle;

  void onConnect(BLEServer* server) override;
  void onDisconnect(BLEServer* server) override;
  void onWrite(BLECharacteristic* characteristic) override;
};
