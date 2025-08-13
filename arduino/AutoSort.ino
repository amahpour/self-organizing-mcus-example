#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Proto.h"
#include "Bus.h"
#include "Node.h"

// Adjust pins to free the primary USB serial for logs
// Example pins for Uno: RX=10, TX=11
static const uint8_t RX_PIN = 10;
static const uint8_t TX_PIN = 11;

Bus bus(RX_PIN, TX_PIN);
Node* node = nullptr;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("AutoSort start");

  randomSeed(analogRead(A0));

  bus.begin(19200);

  static Node localNode(bus, 0); // instance index 0; use 1 on the second board if desired
  node = &localNode;
  node->begin();
}

void loop() {
  node->service();
}
