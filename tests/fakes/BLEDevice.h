#pragma once

#include "Arduino.h"
#include <map>
#include <memory>
#include <vector>

class BLEServer;
class BLECharacteristic;

class BLEServerCallbacks {
public:
  virtual ~BLEServerCallbacks() = default;
  virtual void onConnect(BLEServer*) {}
  virtual void onDisconnect(BLEServer*) {}
};

class BLECharacteristicCallbacks {
public:
  virtual ~BLECharacteristicCallbacks() = default;
  virtual void onWrite(BLECharacteristic*) {}
};

class BLEUUID {
public:
  explicit BLEUUID(uint16_t) {}
};

class BLEDescriptor {
public:
  explicit BLEDescriptor(BLEUUID) {}
  virtual ~BLEDescriptor() = default;
  void setValue(const char*) {}
};

class BLECharacteristic {
public:
  enum { PROPERTY_READ = 1, PROPERTY_WRITE = 2, PROPERTY_NOTIFY = 4 };
  void addDescriptor(BLEDescriptor* descriptor) { descriptors_.emplace_back(descriptor); }
  void setCallbacks(BLECharacteristicCallbacks* callbacks) { callbacks_ = callbacks; }
  void setValue(const uint8_t* bytes, size_t size) {
    value_.assign(reinterpret_cast<const char*>(bytes), size);
  }
  void setValue(const char* text) { value_ = text; }
  String getValue() const { return value_; }
  void notify() { ++notifications; }
  void writeFromClient(const String& value) {
    value_ = value;
    if (callbacks_) callbacks_->onWrite(this);
  }
  unsigned notifications = 0;

private:
  String value_;
  BLECharacteristicCallbacks* callbacks_ = nullptr;
  std::vector<std::unique_ptr<BLEDescriptor>> descriptors_;
};

class BLEService {
public:
  BLECharacteristic* createCharacteristic(const char* uuid, int) {
    auto& characteristic = characteristics_[uuid];
    characteristic.reset(new BLECharacteristic());
    return characteristic.get();
  }
  BLECharacteristic* find(const char* uuid) { return characteristics_.at(uuid).get(); }
  void start() {}

private:
  std::map<String, std::unique_ptr<BLECharacteristic>> characteristics_;
};

class BLEServer {
public:
  void setCallbacks(BLEServerCallbacks* callbacks) { callbacks_ = callbacks; }
  BLEService* createService(const char*) { return &service; }
  void connect() { callbacks_->onConnect(this); }
  void disconnect() { callbacks_->onDisconnect(this); }
  BLEService service;

private:
  BLEServerCallbacks* callbacks_ = nullptr;
};

struct BLEAdvertising {
  unsigned starts = 0;
  void addServiceUUID(const char*) {}
  void setScanResponse(bool) {}
  void start() { ++starts; }
};

extern BLEServer fakeBleServer;
extern BLEAdvertising fakeAdvertising;

class BLEDevice {
public:
  static void init(const char*) { fakeAdvertising = BLEAdvertising(); }
  static BLEServer* createServer() {
    fakeBleServer = BLEServer();
    return &fakeBleServer;
  }
  static BLEAdvertising* getAdvertising() { return &fakeAdvertising; }
  static void startAdvertising() { fakeAdvertising.start(); }
};
