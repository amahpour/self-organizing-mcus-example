/**
 * @file PingPongUno.ino
 * @brief Simple ping-pong test for Arduino Uno using SoftwareSerial
 *
 * This sketch is for the classic Arduino Uno side of the communication.
 * Uses SoftwareSerial on pins 10 (RX) and 11 (TX).
 */

#include <SoftwareSerial.h>

// SoftwareSerial for communication with R4
SoftwareSerial commSerial(10, 11); // RX=10, TX=11

unsigned long lastPingTime = 0;
unsigned long pingInterval = 2000; // Send ping every 2 seconds

void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Arduino Uno Ping Pong Test ===");
  Serial.println("Board: Arduino Uno (classic)");
  Serial.println("Using: SoftwareSerial (pins 10 RX, 11 TX)");
  Serial.println("Baud: 9600");
  Serial.println("==================================");
  
  // Initialize SoftwareSerial for inter-board communication
  commSerial.begin(9600);
  
  Serial.println("SoftwareSerial initialized");
  Serial.println("Starting ping-pong test...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send PING every interval
  if (currentTime - lastPingTime >= pingInterval) {
    commSerial.println("PING");
    Serial.println("Sent: PING");
    lastPingTime = currentTime;
  }
  
  // Check for incoming messages
  if (commSerial.available()) {
    String message = commSerial.readStringUntil('\n');
    message.trim();
    
    Serial.println("*** RECEIVED: '" + message + "' ***");
    
    if (message == "PING") {
      Serial.println("Got PING - sending PONG");
      commSerial.println("PONG");
    } else if (message == "PONG") {
      Serial.println("*** SUCCESS: Got PONG response! ***");
    } else if (message.length() > 0) {
      Serial.println("Unknown message: '" + message + "'");
    }
  }
  
  delay(100);
}
