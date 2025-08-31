/**
 * @file AutoSortR4.ino
 * @brief Simple self-organizing system for Arduino UNO R4 WiFi
 *
 * Uses hardware Serial1 instead of SoftwareSerial for reliable communication.
 */

#include <Arduino.h>

extern "C" {
#include "shared/core/hal.h"
#include "shared/core/bus_interface.h"
#include "shared/core/node.h"
#include "shared/core/proto.c"
#include "shared/core/node.c"
#include "shared/platform/arduino/hal_arduino.c"
}

// Include bus_uno_r4.c here where it can access C++ HardwareSerial
#include "shared/platform/arduino_uno_r4/bus_uno_r4.c"

// Hardware configuration
static const uint8_t RX_PIN = 0;
static const uint8_t TX_PIN = 1;

Bus* bus = nullptr;
Node node;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("AutoSort R4 starting...");
  
  hal_init();
  
  if (bus_create(&bus, 0, RX_PIN, TX_PIN) != 0) {
    Serial.println("Failed to create bus!");
    while (1) { ; }
  }
  
  node_init(&node, bus, 0);  // Instance 0 - starts immediately
  node_begin(&node);
  
  Serial.println("Node initialized and started");
}

void loop() {
  node_service(&node);
  delay(10); // Small service interval
}
