/**
 * @file PingPongR4Simple.ino
 * @brief Simple ping-pong test for Arduino UNO R4 WiFi using Serial1
 *
 * This is a minimal test to verify Serial1 communication between two R4 boards.
 * No complex protocols - just basic string sending and receiving.
 */

unsigned long lastPingTime = 0;
unsigned long pingInterval = 2000; // Send ping every 2 seconds

void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== R4 Simple Ping Pong Test ===");
  Serial.println("Board: Arduino UNO R4 WiFi");
  Serial.println("Using: Serial1 (pins 0 RX, 1 TX)");
  Serial.println("Baud: 9600");
  Serial.println("================================");
  
  // Initialize Serial1 for inter-board communication
  Serial1.begin(9600);  // Lower baud rate for reliability
  
  Serial.println("Serial1 initialized");
  Serial.println("Starting ping-pong test...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send PING every interval
  if (currentTime - lastPingTime >= pingInterval) {
    Serial1.println("PING");
    Serial.println("Sent: PING");
    lastPingTime = currentTime;
  }
  
  // Check for incoming messages
  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    message.trim();
    
    Serial.println("*** RECEIVED: '" + message + "' ***");
    
    if (message == "PING") {
      Serial.println("Got PING - sending PONG");
      Serial1.println("PONG");
    } else if (message == "PONG") {
      Serial.println("*** SUCCESS: Got PONG response! ***");
    } else if (message.length() > 0) {
      Serial.println("Unknown message: '" + message + "'");
    }
  }
  
  delay(100);
}
