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
#elif defined(__AVR_ATmega328P__) && !defined(ARDUINO_AVR_UNO)
  #define BOARD_TYPE "ATMEGA328P"
  #define USE_SOFTWARE_SERIAL
  #include <SoftwareSerial.h>
  #ifndef F_CPU
  #define F_CPU 8000000UL  // 8MHz internal RC oscillator
  #endif
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
  #if defined(__AVR_ATmega328P__) && !defined(ARDUINO_AVR_UNO)
    static const uint8_t INSTANCE_INDEX = 2;  // Atmega328P starts with delay
    static const uint32_t DEBUG_BAUD = 38400;  // Lower baud for 8MHz internal RC
    static const uint32_t BUS_BAUD = 4800;     // Lower baud for SoftwareSerial at 8MHz
  #else
    static const uint8_t INSTANCE_INDEX = 1;  // UNO starts with delay
    static const uint32_t DEBUG_BAUD = 115200; // Standard Arduino baud rate
    static const uint32_t BUS_BAUD = 9600;     // Standard SoftwareSerial baud rate
  #endif
#else
  #include "shared/platform/arduino_uno_r4/bus_uno_r4.c"
  static const uint8_t RX_PIN = 0;
  static const uint8_t TX_PIN = 1;
  static const uint8_t INSTANCE_INDEX = 0;  // R4 starts immediately
  static const uint32_t DEBUG_BAUD = 115200; // Standard Arduino baud rate
  static const uint32_t BUS_BAUD = 9600;     // Not used for R4 (hardware serial)
#endif

Bus* bus = nullptr;
Node node;

void setup() {
  Serial.begin(DEBUG_BAUD);
  delay(2000);
  
  Serial.println("AutoSort " BOARD_TYPE " starting...");
  
  hal_init();
  
  if (bus_create(&bus, 0, RX_PIN, TX_PIN) != 0) {
    Serial.println("Failed to create bus!");
    while (1) { ; }
  }
  
  // Set board-specific baud rate for SoftwareSerial
  #ifdef USE_SOFTWARE_SERIAL
    bus_set_baud(bus, BUS_BAUD);
  #endif
  
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