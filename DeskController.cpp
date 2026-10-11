#include "DeskController.h"
#include "Config.h"

void DeskController::begin() {
  Serial.begin(Config::DebugBaud);
  proxy.begin();
  Serial.println("Desk serial bridge ready");
}

void DeskController::loop() {

  for (unsigned count = 0; count < Config::BytesPerLoop; ++count) {
    int byte = proxy.readKeypad();
    if (byte < 0) break;
    proxy.sendToController(static_cast<uint8_t>(byte));
    if (Serial) Serial.printf("KEY -> %02X\n", byte);
  }
  for (unsigned count = 0; count < Config::BytesPerLoop; ++count) {
    int byte = proxy.readController();
    if (byte < 0) break;
    proxy.sendToKeypad(static_cast<uint8_t>(byte));
    if (Serial) Serial.printf("CTRL -> %02X\n", byte);
  }
}
