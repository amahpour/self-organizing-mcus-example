/**
 * @file ProtocolTestUno.ino
 * @brief Test binary protocol frames between Uno and R4
 */

#include <SoftwareSerial.h>

SoftwareSerial commSerial(10, 11); // RX=10, TX=11

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Protocol Test - Arduino Uno ===");
  commSerial.begin(9600);
  
  Serial.println("Sending test binary frame...");
  
  // Send a simple binary frame similar to AutoSort protocol
  uint8_t frame[] = {0xAA, 0x02, 0x01, 0x04, 0x12, 0x34, 0x56, 0x78, 0x00}; // SOF, type, source, len, payload, checksum
  
  // Calculate simple checksum
  uint8_t checksum = 0;
  for (int i = 1; i < 8; i++) {
    checksum ^= frame[i];
  }
  frame[8] = checksum;
  
  commSerial.write(frame, 9);
  Serial.println("Frame sent:");
  for (int i = 0; i < 9; i++) {
    Serial.print("0x");
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void loop() {
  // Check for incoming binary data
  if (commSerial.available()) {
    Serial.print("Received byte: 0x");
    uint8_t b = commSerial.read();
    Serial.println(b, HEX);
  }
  
  delay(100);
}
