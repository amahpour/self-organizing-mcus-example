#include <Arduino.h>

extern "C" {
#include "shared/core/hal.h"
#include "shared/core/bus_interface.h"
#include "shared/core/node.h"
#include "shared/core/proto.c"
#include "shared/core/node.c"
#include "shared/platform/arduino/hal_arduino.c"
#include "shared/platform/arduino/bus_arduino.c"
}

// Hardware configuration
static const uint8_t RX_PIN = 10;
static const uint8_t TX_PIN = 11;

Bus* bus = nullptr;
Node node;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  
  Serial.println("AutoSort Arduino starting...");
  
  hal_init();
  
  if (bus_create(&bus, 0, RX_PIN, TX_PIN) != 0) {
    Serial.println("Failed to create bus!");
    while (1) { ; }
  }
  
  node_init(&node, bus, 0);
  node_begin(&node);
  
  Serial.println("Node initialized and started");
}

void loop() {
  node_service(&node);
  delay(10); // Small service interval
}
