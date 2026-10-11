#include "DeskController.h"
#include "Config.h"

void DeskController::begin() {
  Serial.begin(Config::DebugBaud);
  proxy.begin();
  Serial.println("Desk frame monitor ready");
}

void DeskController::loop() {
  const uint32_t now = millis();
  keypadFrames.expire(now);
  controllerFrames.expire(now);
  Frame frame;

  for (unsigned count = 0; count < Config::BytesPerLoop; ++count) {
    int byte = proxy.readKeypad();
    if (byte < 0) break;
    proxy.sendToController(static_cast<uint8_t>(byte));
    if (keypadFrames.addByte(static_cast<uint8_t>(byte), millis(), frame)) {
      logger.logFrame(frame);
    }
  }
  for (unsigned count = 0; count < Config::BytesPerLoop; ++count) {
    int byte = proxy.readController();
    if (byte < 0) break;
    proxy.sendToKeypad(static_cast<uint8_t>(byte));
    if (controllerFrames.addByte(static_cast<uint8_t>(byte), millis(), frame)) {
      logger.logFrame(frame);
    }
  }
}
