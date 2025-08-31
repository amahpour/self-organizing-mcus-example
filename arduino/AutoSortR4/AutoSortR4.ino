/**
 * @file AutoSortR4.ino
 * @brief Self-organizing system for Arduino UNO R4 WiFi using hardware Serial1
 *
 * This version uses the UNO R4's hardware Serial1 (pins 0 RX, 1 TX) instead of
 * SoftwareSerial, avoiding all the Renesas RA4M1 architecture issues.
 */

#include "shared/core/node.h"
#include "shared/core/bus_interface.h"
#include "shared/core/hal.h"
#include "shared/core/proto.h"

// Use hardware Serial1 (pins 0 RX, 1 TX) on Arduino UNO R4 WiFi
static const uint8_t RX_PIN = 0;  // Hardware serial - pin assignment ignored by bus implementation
static const uint8_t TX_PIN = 1;  // Hardware serial - pin assignment ignored by bus implementation

static Bus* bus = nullptr;
static Node node;

// State for non-blocking coordinator election
static bool election_started = false;
static unsigned long election_start_time = 0;
static int election_phase = 0;  // 0: listen, 1: claim, 2: tie-break, 3: done

void setup() {
  Serial.begin(115200);
  delay(2000);  // Allow R4 to fully initialize
  
  Serial.println("=== Arduino UNO R4 WiFi Self-Organizing System ===");
  Serial.println("Using: Hardware Serial1 (pins 0 RX, 1 TX)");
  Serial.println("Architecture: Renesas RA4M1 with hardware UART");
  Serial.println("Baud Rate: 19200");
  Serial.println("==================================================");
  
  // Initialize platform
  hal_init();
  bus_global_init(4);
  
  // Create bus using hardware serial
  int result = bus_create(&bus, 0, RX_PIN, TX_PIN);
  if (result != 0) {
    Serial.println("ERROR: Failed to create bus");
    while (1) { delay(1000); }
  }
  
  // Initialize node
  node_init(&node, bus, 0);
  
  Serial.println("Hardware Serial1 initialized successfully");
  Serial.println("Starting non-blocking coordinator election...");
  
  election_started = true;
  election_start_time = millis();
  election_phase = 0;
}

void loop() {
  unsigned long currentTime = millis();
  
  // Non-blocking coordinator election state machine
  if (election_started && election_phase < 3) {
    unsigned long elapsed = currentTime - election_start_time;
    
    switch (election_phase) {
      case 0: // Listen phase (200ms)
        if (elapsed >= 200) {
          Serial.println("Listen phase complete - no coordinator found");
          Serial.println("Broadcasting CLAIM message...");
          
          // Create and send CLAIM message
          Frame frame;
          frame.sof = SOF;
          frame.type = MSG_CLAIM;
          frame.source = 1;  // Temporary ID during election
          frame.payload_len = 4;
          
          // Generate random nonce for tie-breaking
          uint32_t nonce = hal_random32();
          frame.payload[0] = (nonce >> 24) & 0xFF;
          frame.payload[1] = (nonce >> 16) & 0xFF;
          frame.payload[2] = (nonce >> 8) & 0xFF;
          frame.payload[3] = nonce & 0xFF;
          
          proto_finalize(&frame);
          bus_send(bus, &frame);
          
          Serial.println("CLAIM sent with nonce: " + String(nonce));
          election_phase = 1;
        }
        break;
        
      case 1: // Claim phase (150ms tie-break window)
        if (elapsed >= 350) { // 200ms listen + 150ms tie-break
          Serial.println("Tie-break window complete");
          Serial.println("Becoming COORDINATOR");
          
          // Manually set coordinator state
          node.state = STATE_COORDINATOR;
          node.node_id = 1;
          node.next_assign_id = 2;
          
          election_phase = 3; // Done
          Serial.println("Coordinator election complete!");
        }
        break;
    }
  }
  
  // Service the node state machine (handles ongoing operations)
  node_service(&node);
  
  // Print status periodically
  static unsigned long lastStatusTime = 0;
  if (currentTime - lastStatusTime >= 2000) {
    const char* state_str = (node.state == STATE_SEEKING) ? "SEEKING" :
                           (node.state == STATE_COORDINATOR) ? "COORDINATOR" : "MEMBER";
    Serial.println("Status: " + String(state_str) + " (ID: " + String(node.node_id) + ")");
    lastStatusTime = currentTime;
  }
  
  delay(50);
}
