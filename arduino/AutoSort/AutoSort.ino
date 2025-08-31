#include <Arduino.h>

// Include C++ headers first (before extern "C")
#include <SoftwareSerial.h>

extern "C" {
#include "shared/core/hal.h"
#include "shared/core/bus_interface.h"
#include "shared/core/node.h"
#include "shared/core/proto.h"
#include "shared/core/proto.c"
#include "shared/core/node.c"
#include "shared/platform/arduino/hal_arduino.c"
// Note: bus_arduino.c included after extern "C" block due to SoftwareSerial dependency
}

// Include bus_arduino.c here where it can access C++ SoftwareSerial
#include "shared/platform/arduino/bus_arduino.c"

// Hardware configuration
static const uint8_t RX_PIN = 10;
static const uint8_t TX_PIN = 11;

Bus* bus = nullptr;
Node node;

void setup() {
  Serial.begin(115200);
  
  delay(2000);
  
  Serial.println("AutoSort Arduino starting...");
  
  hal_init();
  
  if (bus_create(&bus, 0, RX_PIN, TX_PIN) != 0) {
    Serial.println("Failed to create bus!");
    while (1) { ; }
  }
  
  node_init(&node, bus, 0);
  
  // Don't call node_begin() here - it blocks
  // Instead, set up for non-blocking operation
  node.role = NODE_SEEKING;
  node.assigned_id = 0;
  node.random_nonce = hal_random32();
  node.seen_count = 0;
  node.last_join_ms = 0;
  
  Serial.println("Node initialized - starting coordinator election...");
}

// Global state for non-blocking coordinator election
static uint32_t election_start_time = 0;
static uint8_t election_phase = 0; // 0=not started, 1=listening, 2=claiming, 3=conflict detection, 4=complete

void loop() {
  // Handle non-blocking coordinator election
  if (node.role == NODE_SEEKING && election_phase == 0) {
    // Start election process
    election_start_time = hal_millis();
    election_phase = 1;
    Serial.println("Starting coordinator election...");
  }
  
  if (election_phase == 1) {
    // Phase 1: Listen for existing CLAIM messages (200ms window)
    if ((hal_millis() - election_start_time) >= 200) {
      // No CLAIM heard - proceed to claim
      election_phase = 2;
      election_start_time = hal_millis();
      
      // Send CLAIM
      uint8_t payload[4];
      u32_to_bytes(node.random_nonce, payload);
      Frame claim;
      claim.type = MSG_CLAIM;
      claim.source = 0;
      claim.payload_len = 4;
      memcpy(claim.payload, payload, 4);
      proto_finalize(&claim);
      bus_send(bus, &claim);
      
      char msg[64];
      snprintf(msg, sizeof(msg), "Node[%u] CLAIM nonce=%u", node.instance_index, node.random_nonce);
      Serial.println(msg);
    } else {
      // Still listening - check for CLAIM messages
      Frame in;
      if (bus_recv(bus, &in, 10) && proto_is_valid(&in) && in.type == MSG_CLAIM) {
        // Heard a CLAIM - become member
        election_phase = 4;
        node.role = NODE_SEEKING; // Will become member
        Serial.println("Heard CLAIM - will become member");
      }
    }
  }
  
  if (election_phase == 2) {
    // Phase 2: Conflict detection window (150ms)
    if ((hal_millis() - election_start_time) >= 150) {
      // No conflict - become coordinator
      election_phase = 4;
      node.role = NODE_COORDINATOR;
      node.assigned_id = 1;
      node.next_assign_id = 2;
      
      char msg[64];
      snprintf(msg, sizeof(msg), "Node[%u] → COORDINATOR (ID=1)", node.instance_index);
      Serial.println(msg);
    } else {
      // Still in conflict detection - check for higher nonce
      Frame in;
      if (bus_recv(bus, &in, 10) && proto_is_valid(&in) && in.type == MSG_CLAIM && in.payload_len >= 4) {
        uint32_t other_nonce = bytes_to_u32(in.payload);
        if (other_nonce > node.random_nonce) {
          // Lost to higher nonce - become member
          election_phase = 4;
          node.role = NODE_SEEKING; // Will become member
          Serial.println("Lost election - will become member");
        }
      }
    }
  }
  
  if (election_phase == 4 && node.role == NODE_SEEKING) {
    // Start member joining process
    election_phase = 5;
    
    // Send HELLO
    Frame hello;
    hello.type = MSG_HELLO;
    hello.source = 0;
    hello.payload_len = 0;
    proto_finalize(&hello);
    bus_send(bus, &hello);
    Serial.println("HELLO");
    
    // Send JOIN
    node.join_nonce = hal_random32();
    uint8_t payload[4];
    u32_to_bytes(node.join_nonce, payload);
    Frame join;
    join.type = MSG_JOIN;
    join.source = 0;
    join.payload_len = 4;
    memcpy(join.payload, payload, 4);
    proto_finalize(&join);
    bus_send(bus, &join);
    node.last_join_ms = hal_millis();
    
    char msg[64];
    snprintf(msg, sizeof(msg), "JOIN (nonce=%u)", node.join_nonce);
    Serial.println(msg);
  }
  
  // Service the node state machine
  node_service(&node);
  delay(10); // Small service interval
}
