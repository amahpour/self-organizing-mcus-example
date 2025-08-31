/**
 * @file ProtocolTestR4.ino
 * @brief Test binary protocol frames between Uno and R4
 */

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== Protocol Test - Arduino R4 ===");
  Serial1.begin(9600);
  
  Serial.println("Sending test binary frame...");
  
  // Send a simple binary frame similar to AutoSort protocol
  uint8_t frame[] = {0xAA, 0x01, 0x02, 0x04, 0xAB, 0xCD, 0xEF, 0x12, 0x00}; // SOF, type, source, len, payload, checksum
  
  // Calculate simple checksum
  uint8_t checksum = 0;
  for (int i = 1; i < 8; i++) {
    checksum ^= frame[i];
  }
  frame[8] = checksum;
  
  Serial1.write(frame, 9);
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
  if (Serial1.available()) {
    Serial.print("Received byte: 0x");
    uint8_t b = Serial1.read();
    Serial.println(b, HEX);
  }
  
  delay(100);
}
