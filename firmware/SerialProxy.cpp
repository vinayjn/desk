#include "SerialProxy.h"
#include "Config.h"

#if !defined(CONFIG_IDF_TARGET_ESP32C3)
#error "This sketch targets the ESP32-C3."
#endif
#if !ARDUINO_USB_CDC_ON_BOOT
#error "Enable USB CDC On Boot: UART0 is used for the desk controller."
#endif

void SerialProxy::begin() {
  keypad.end();
  // Controller replies reach keypad RX directly; this UART only receives.
  keypad.begin(Config::DeskBaud, SERIAL_8N1, Config::KeypadRxPin, -1);
  controller.begin(Config::DeskBaud, SERIAL_8N1,
                   Config::ControllerRxPin, Config::ControllerTxPin);
}

int SerialProxy::readKeypad() { return keypad.read(); }
int SerialProxy::readController() { return controller.read(); }
void SerialProxy::sendToController(uint8_t byte) { controller.write(byte); }
void SerialProxy::sendToController(const Frame& frame) {
  controller.write(frame.bytes().data(), frame.bytes().size());
}
