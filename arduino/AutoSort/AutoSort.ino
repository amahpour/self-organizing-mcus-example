/**
 * @file AutoSort.ino
 * @brief Universal self-organizing system for Arduino boards
 *
 * Automatically detects board type and uses appropriate communication:
 * - Arduino UNO: SoftwareSerial (pins 10/11)
 * - Arduino UNO R4 WiFi: HardwareSerial1 (pins 0/1)
 */

#include <Arduino.h>

// Detect board type at compile time
#if defined(ARDUINO_UNOR4_WIFI)
  #define BOARD_TYPE "R4"
  #define USE_HARDWARE_SERIAL
#else
  #define BOARD_TYPE "UNO"
  #define USE_SOFTWARE_SERIAL
  #include <SoftwareSerial.h>
#endif

extern "C" {
#include "shared/core/hal.h"
#include "shared/core/bus_interface.h"
#include "shared/core/node.h"
#include "shared/core/proto.c"
#include "shared/core/node.c"
#include "shared/platform/arduino/hal_arduino.c"
}

// Include appropriate bus implementation
#ifdef USE_SOFTWARE_SERIAL
  #include "shared/platform/arduino/bus_arduino.c"
  static const uint8_t RX_PIN = 10;
  static const uint8_t TX_PIN = 11;
  static const uint8_t INSTANCE_INDEX = 1;  // UNO starts with delay
#else
  #include "shared/platform/arduino_uno_r4/bus_uno_r4.c"
  static const uint8_t RX_PIN = 0;
  static const uint8_t TX_PIN = 1;
  static const uint8_t INSTANCE_INDEX = 0;  // R4 starts immediately
#endif

Bus* bus = nullptr;
Node node;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("AutoSort " BOARD_TYPE " starting...");
  
  hal_init();
  
  if (bus_create(&bus, 0, RX_PIN, TX_PIN) != 0) {
    Serial.println("Failed to create bus!");
    while (1) { ; }
  }
  
  Serial.println("DEBUG: [" BOARD_TYPE "] About to init node with instance " + String(INSTANCE_INDEX));
  node_init(&node, bus, INSTANCE_INDEX);
  
  Serial.println("DEBUG: [" BOARD_TYPE "] About to call node_begin()");
  unsigned long start_time = millis();
  node_begin(&node);
  unsigned long end_time = millis();
  
  Serial.println("DEBUG: [" BOARD_TYPE "] node_begin() took " + String(end_time - start_time) + "ms");
  Serial.println("Node initialized and started");
}

void loop() {
  node_service(&node);
  delay(10); // Small service interval
}