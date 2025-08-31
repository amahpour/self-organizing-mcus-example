/**
 * @file AutoSortDue.ino
 * @brief Self-organizing system for Arduino Due using hardware Serial1
 *
 * This version uses the Arduino Due's hardware serial ports instead of
 * SoftwareSerial, providing much more reliable communication.
 */

#include <Arduino.h>

extern "C" {
#include "shared/core/hal.h"
#include "shared/core/bus_interface.h"
#include "shared/core/node.h"
#include "shared/core/proto.h"
#include "shared/core/proto.c"
#include "shared/core/node.c"
#include "shared/platform/arduino/hal_arduino.c"
}

// Include bus_due.c here where it can access C++ HardwareSerial
#include "shared/platform/arduino_due/bus_due.c"

// Use hardware Serial1 (pins 19 RX, 18 TX) on Arduino Due
static const uint8_t RX_PIN = 19;  // Hardware serial - pin assignment ignored
static const uint8_t TX_PIN = 18;  // Hardware serial - pin assignment ignored

static Bus* bus = nullptr;
static Node node;

// State for non-blocking coordinator election
static bool election_started = false;
static unsigned long election_start_time = 0;
static int election_phase = 0;  // 0: listen, 1: claim, 2: tie-break, 3: done

void setup() {
  Serial.begin(115200);
  delay(2000);  // Allow board to fully initialize
  
  Serial.println("=== Arduino Due Self-Organizing System ===");
  Serial.println("Using: Hardware Serial1 (pins 19 RX, 18 TX)");
  Serial.println("Baud Rate: 19200");
  Serial.println("==========================================");
  
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
  
  Serial.println("Hardware Serial1 initialized");
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
          node.role = NODE_COORDINATOR;
          node.assigned_id = 1;
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
    const char* state_str = (node.role == NODE_SEEKING) ? "SEEKING" :
                           (node.role == NODE_COORDINATOR) ? "COORDINATOR" : "MEMBER";
    Serial.println("Status: " + String(state_str) + " (ID: " + String(node.assigned_id) + ")");
    lastStatusTime = currentTime;
  }
  
  delay(50);
}
