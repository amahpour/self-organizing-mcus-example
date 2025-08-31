#include <SoftwareSerial.h>

// Hardware configuration
static const uint8_t RX_PIN = 10;
static const uint8_t TX_PIN = 11;

SoftwareSerial softSerial(RX_PIN, TX_PIN, false); // false = normal logic

unsigned long lastPingTime = 0;
unsigned long pingInterval = 3000; // Send ping every 3 seconds
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
  
  // Check for incoming messages with aggressive debugging
  if (softSerial.available()) {
    int bytesAvailable = softSerial.available();
    Serial.println("*** DATA DETECTED! Bytes available: " + String(bytesAvailable) + " ***");
    
    String message = softSerial.readStringUntil('\n');
    message.trim();
    
    Serial.println("Raw message: '" + message + "' (length: " + String(message.length()) + ")");
    
    if (message == "PING") {
      Serial.println("Received: PING");
      Serial.println("Sending: PONG");
      softSerial.println("PONG");
      lastReceivedTime = currentTime;
    } else if (message == "PONG") {
      Serial.println("*** SUCCESS: Received PONG! ***");
      lastReceivedTime = currentTime;
    } else if (message.length() > 0) {
      Serial.println("Received unknown: '" + message + "'");
      lastReceivedTime = currentTime;
    } else {
      Serial.println("Received empty message");
    }
  } else {
    // Debug: show when no data is available
    static unsigned long lastDebugTime = 0;
    if (currentTime - lastDebugTime >= 3000) { // Every 3 seconds
      Serial.println("Debug: No data available from SoftwareSerial (pin " + String(RX_PIN) + ")");
      lastDebugTime = currentTime;
    }
  }
  
  // Check for timeout
  if (lastReceivedTime > 0 && currentTime - lastReceivedTime >= timeout) {
    Serial.println("WARNING: No communication detected for " + String(timeout/1000) + " seconds");
    lastReceivedTime = 0; // Reset to avoid spam
  }
  
  delay(100); // Small delay to prevent overwhelming
}
