#include "DeskController.h"
#include "Config.h"

void DeskController::begin() {
  Serial.begin(Config::DebugBaud);
  proxy.begin();
  startedAt_ = millis();
  Serial.println("Desk height monitor ready; send q to query height");
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
      onControllerFrame(frame, millis());
    }
  }
  for (unsigned count = 0; count < Config::BytesPerLoop && Serial.available(); ++count) {
    if (Serial.read() == 'q') queryRequested_ = true;
  }
  if (queryRequested_ && !keypadFrames.isBuilding() &&
      uint32_t(millis() - startedAt_) >= Config::StartupQueryDelayMs) {
    proxy.sendToController(Frame::queryCommand());
    queryRequested_ = false;
  }
}

void DeskController::onControllerFrame(const Frame& frame, uint32_t now) {
  if (frame.kind() != Frame::Kind::Height) return;
  if (state.updateHeight(frame.decodeHeight(), now) && Serial) {
    Serial.printf("Height: %d cm\n", state.currentHeight());
  }
}
