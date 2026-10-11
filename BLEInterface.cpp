#include "BLEInterface.h"
#include <BLE2902.h>
#include <BLEUtils.h>

void BLEInterface::begin() {
  BLEDevice::init(Config::BleDeviceName);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(this);
  BLEService* service = server->createService(Config::BleServiceUuid);

  height_ = service->createCharacteristic(Config::BleHeightUuid,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  height_->addDescriptor(new BLE2902());
  auto* heightDescription = new BLEDescriptor(BLEUUID(uint16_t(0x2901)));
  heightDescription->setValue("char_current_height|Current Height (cm)");
  height_->addDescriptor(heightDescription);

  target_ = service->createCharacteristic(Config::BleTargetUuid, BLECharacteristic::PROPERTY_WRITE);
  target_->setCallbacks(this);
  auto* targetDescription = new BLEDescriptor(BLEUUID(uint16_t(0x2901)));
  targetDescription->setValue("char_set_target_height|Target Height (cm)");
  target_->addDescriptor(targetDescription);

  status_ = service->createCharacteristic(Config::BleStatusUuid,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  status_->addDescriptor(new BLE2902());
  auto* statusDescription = new BLEDescriptor(BLEUUID(uint16_t(0x2901)));
  statusDescription->setValue("char_desk_status|Desk Status (IDLE, MOVING, ERROR)");
  status_->addDescriptor(statusDescription);

  publish(-1, DeskState::Status::Idle);
  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(Config::BleServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();
}

void BLEInterface::onConnect(BLEServer*) {
  connected_.store(true);
  queryRequested_.store(true);
  publishRequested_.store(true);
}

void BLEInterface::onDisconnect(BLEServer*) {
  connected_.store(false);
  pendingTarget_.store(-1);
  stopRequested_.store(true);
  restartAdvertising_.store(true);
}

void BLEInterface::onWrite(BLECharacteristic* characteristic) {
  if (characteristic != target_ || !connected_.load()) return;
  const String value = characteristic->getValue();

  pendingTarget_.store(value.length() == 1 ? uint8_t(value[0]) : 0);
}

void BLEInterface::poll() {
  if (restartAdvertising_.exchange(false)) BLEDevice::startAdvertising();
}

void BLEInterface::publish(int height, DeskState::Status status) {
  const bool force = publishRequested_.exchange(false);
  if (force || height != lastHeight_) {
    const uint8_t value = height >= 0 ? static_cast<uint8_t>(height) : 0;
    height_->setValue(&value, 1);
    lastHeight_ = height;
    if (connected_.load()) height_->notify();
  }
  if (force || status != lastStatus_) {
    status_->setValue(DeskState::statusName(status));
    lastStatus_ = status;
    if (connected_.load()) status_->notify();
  }
}
