// Keypad red/TX -> GPIO1; GPIO7 -> controller RX.
// Controller TX -> GPIO6 AND keypad black/RX.
// Yellow: existing VCC supply. White: shared GND.
#include "DeskController.h"

DeskController desk;

void setup() {
  desk.begin();
}

void loop() {
  desk.loop();
}
