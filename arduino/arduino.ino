#include <Arduino.h>
extern "C" {
#include "../shared/hal.h"
#include "../shared/bus.h"
#include "../shared/node.h"
#include "../shared/hal_arduino.cpp"
#include "../shared/bus_arduino.cpp"
#include "../shared/node.c"
}

static const uint8_t RX_PIN = 10;
static const uint8_t TX_PIN = 11;

Bus bus;
Node node;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  hal_init();
  bus_init(&bus, 0, RX_PIN, TX_PIN);
  node_init(&node, &bus, 0);
  node_begin(&node);
}

void loop() {
  node_service(&node);
}
