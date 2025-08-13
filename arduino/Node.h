#pragma once
#include <Arduino.h>
#include "Proto.h"
#include "Bus.h"

enum NodeRole : uint8_t {
  NODE_SEEKING = 0,
  NODE_COORDINATOR = 1,
  NODE_MEMBER = 2
};

class Node {
 public:
  Node(Bus& b, uint8_t instanceIndex) : bus(b), instanceIndex(instanceIndex) {}

  void begin() {
    role = NODE_SEEKING;
    assignedId = 0;
    randomNonce = ((uint32_t)random(0xFFFF)) << 16 ^ (uint32_t)random(0xFFFF);
    seenCount = 0;
    lastJoinMs = 0;

    // Initial: small delay based on instanceIndex (manual stagger if desired)
    delay((unsigned long)instanceIndex * 150);

    // Listen briefly for CLAIM
    Frame in;
    bool heard = false;
    unsigned long endAt = millis() + 200;
    while (millis() < endAt) {
      if (bus.recv(in, 20) && in.type == MSG_CLAIM) { heard = true; break; }
      yield();
    }

    if (!heard) {
      // Claim coordinator
      uint8_t p[4]; u32_to_bytes(randomNonce, p);
      Frame claim { SOF, MSG_CLAIM, 0, 4, {0}, 0 };
      memcpy(claim.payload, p, 4);
      proto_finalize(claim);
      bus.send(claim);
      Serial.print("CLAIM nonce="); Serial.println(randomNonce);

      // Small conflict window
      bool lost = false;
      endAt = millis() + 150;
      while (millis() < endAt) {
        if (bus.recv(in, 20) && in.type == MSG_CLAIM && in.payload_len >= 4) {
          if (bytes_to_u32(in.payload) > randomNonce) { lost = true; break; }
        }
        yield();
      }
      if (!lost) {
        role = NODE_COORDINATOR; assignedId = 1; nextAssignId = 2;
        Serial.println("→ COORDINATOR (ID=1)");
      }
    }

    if (role == NODE_SEEKING) {
      // Send HELLO + JOIN and wait for ASSIGN
      Frame hello { SOF, MSG_HELLO, 0, 0, {0}, 0 }; proto_finalize(hello);
      bus.send(hello);
      Serial.println("HELLO");

      joinNonce = ((uint32_t)random(0xFFFF)) << 16 ^ (uint32_t)random(0xFFFF);
      uint8_t p[4]; u32_to_bytes(joinNonce, p);
      Frame join { SOF, MSG_JOIN, 0, 4, {0}, 0 }; memcpy(join.payload, p, 4); proto_finalize(join);
      bus.send(join);
      lastJoinMs = millis();
      Serial.print("JOIN (nonce="); Serial.print(joinNonce); Serial.println(")");
    }
  }

  void service() {
    Frame in;
    if (bus.recv(in, 50)) {
      if (!proto_is_valid(in)) return;
      if (role == NODE_COORDINATOR) {
        if (in.type == MSG_JOIN && in.payload_len >= 4) {
          uint32_t nonce = bytes_to_u32(in.payload);
          if (seen(nonce)) return;
          uint8_t id = nextAssignId++;
          Frame assign { SOF, MSG_ASSIGN, 1, 5, {0}, 0 };
          assign.payload[0] = id;
          memcpy(&assign.payload[1], in.payload, 4);
          proto_finalize(assign);
          bus.send(assign);
          Serial.print("ASSIGN → id="); Serial.println(id);
        }
      } else if (role == NODE_SEEKING) {
        if (in.type == MSG_ASSIGN && in.payload_len >= 5) {
          uint8_t id = in.payload[0];
          uint32_t echoed = bytes_to_u32(&in.payload[1]);
          if (echoed == joinNonce) {
            assignedId = id; role = NODE_MEMBER;
            Serial.print("ASSIGN received → MEMBER (ID="); Serial.print(assignedId); Serial.println(")");
          }
        }
      }
    }

    // Re-send JOIN every 250ms until assigned
    if (role == NODE_SEEKING && (millis() - lastJoinMs) >= 250) {
      uint8_t p[4]; u32_to_bytes(joinNonce, p);
      Frame join { SOF, MSG_JOIN, 0, 4, {0}, 0 }; memcpy(join.payload, p, 4); proto_finalize(join);
      bus.send(join);
      lastJoinMs = millis();
    }
  }

 private:
  Bus& bus;
  uint8_t instanceIndex;
  NodeRole role = NODE_SEEKING;
  uint8_t assignedId = 0;
  uint32_t randomNonce = 0;
  uint8_t nextAssignId = 2;

  uint32_t dedup[32];
  uint8_t seenCount = 0;

  uint32_t joinNonce = 0;
  unsigned long lastJoinMs = 0;

  bool seen(uint32_t nonce) {
    for (uint8_t i = 0; i < seenCount; ++i) if (dedup[i] == nonce) return true;
    if (seenCount < 32) dedup[seenCount++] = nonce; else dedup[seenCount % 32] = nonce;
    return false;
  }
};
