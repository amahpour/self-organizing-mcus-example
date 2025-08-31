#include <SoftwareSerial.h>

// Hardware configuration
static const uint8_t RX_PIN = 10;
static const uint8_t TX_PIN = 11;

SoftwareSerial softSerial(RX_PIN, TX_PIN, false); // false = normal logic

unsigned long lastPingTime = 0;
unsigned long pingInterval = 2000; // Send ping every 2 seconds
unsigned long lastReceivedTime = 0;
unsigned long timeout = 5000; // 5 second timeout

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== Ping Pong Test ===");
  Serial.println("Board: Arduino");
  Serial.println("RX Pin: " + String(RX_PIN));
  Serial.println("TX Pin: " + String(TX_PIN));
  Serial.println("Baud Rate: 19200");
  Serial.println("=====================");
  
  softSerial.begin(19200);
  
  Serial.println("SoftwareSerial initialized");
  Serial.println("Waiting for communication...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send ping every interval
  if (currentTime - lastPingTime >= pingInterval) {
    softSerial.println("PING");
    Serial.println("Sent: PING");
    lastPingTime = currentTime;
  }
  
  // Check for incoming messages
  if (softSerial.available()) {
    String message = softSerial.readStringUntil('\n');
    message.trim();
    
    if (message == "PING") {
      Serial.println("Received: PING");
      Serial.println("Sending: PONG");
      softSerial.println("PONG");
      lastReceivedTime = currentTime;
    } else if (message == "PONG") {
      Serial.println("Received: PONG");
      lastReceivedTime = currentTime;
    } else {
      Serial.println("Received: " + message);
    }
  }
  
  // Check for timeout
  if (lastReceivedTime > 0 && currentTime - lastReceivedTime >= timeout) {
    Serial.println("WARNING: No communication detected for " + String(timeout/1000) + " seconds");
    lastReceivedTime = 0; // Reset to avoid spam
  }
  
  delay(100); // Small delay to prevent overwhelming
}
