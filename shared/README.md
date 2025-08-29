# Shared Library - Self-Organizing Microcontroller System

This directory contains a platform-agnostic distributed coordination system that enables microcontrollers to automatically elect a coordinator and organize into a network without central configuration.

## Architecture Overview

The system uses a clean layered architecture with zero preprocessor conditionals in the core logic:

```
┌─────────────────────────────────────┐
│           Application               │
├─────────────────────────────────────┤
│      Core Business Logic            │
│   (node.c, proto.c - no #ifdefs)    │
├─────────────────────────────────────┤
│     Abstract Interfaces             │
│   (bus_interface.h, hal.h)          │
├─────────────────────────────────────┤
│    Platform Implementations         │
│  (arduino/, sim/ - platform code)   │
└─────────────────────────────────────┘
```

## Core Directory (`core/`)

### State Machine (`node.h`, `node.c`)
The heart of the distributed system. Implements a three-state machine:
- **SEEKING**: Node looks for coordinator or tries to become one
- **COORDINATOR**: Manages network and assigns IDs to new members  
- **MEMBER**: Has received ID and participates in network

**Key Functions:**
- `node_init()` - Initialize with bus and instance index
- `node_begin()` - Start coordinator election (blocking ~400ms)
- `node_service()` - Service state machine (call regularly, non-blocking)

### Communication Protocol (`proto.h`, `proto.c`)
Defines wire protocol for inter-node messaging:
- **Frame Format**: `[SOF][Type][Source][PayloadLen][Payload][Checksum]` (5-13 bytes)
- **Message Types**: HELLO(1), CLAIM(2), JOIN(3), ASSIGN(4), HEARTBEAT(5)
- **Features**: XOR checksum, big-endian byte order, 8-byte max payload

### Bus Interface (`bus_interface.h`)
Abstract communication layer supporting both point-to-point and broadcast:
- `bus_create()` - Create bus instance for a node
- `bus_send()` - Transmit frame to other nodes
- `bus_recv()` - Receive frame with timeout

### Hardware Abstraction (`hal.h`)
Minimal platform abstraction for essential services:
- `hal_millis()` - Monotonic millisecond counter
- `hal_delay()` - Blocking delay for startup jitter
- `hal_random32()` - 32-bit random numbers for tie-breaking
- `hal_log()` - Platform-appropriate logging

## Platform Directory (`platform/`)

### Arduino Implementation (`arduino/`)
- **`bus_arduino.c`**: UART communication via SoftwareSerial (19200 baud)
- **`hal_arduino.c`**: Maps to Arduino functions (`millis()`, `delay()`, etc.)

### Simulation Implementation (`sim/`)  
- **`bus_sim.c`**: Pthread-based message queues with broadcast
- **`hal_sim.c`**: POSIX timing and standard library functions

## Distributed Algorithm

1. **Startup Jitter**: Each node delays `150ms * instance_index` to avoid conflicts
2. **Coordinator Election**: 
   - Listen for existing CLAIM messages (200ms)
   - If none heard, broadcast CLAIM with random nonce
   - Handle tie-breaking using nonce comparison (150ms window)
   - Highest nonce wins coordinator role
3. **Member Joining**:
   - Send HELLO announcement
   - Send JOIN request with unique nonce
   - Retry every 250ms until ASSIGN received
4. **ID Assignment**:
   - Coordinator assigns sequential IDs (starting from 2)
   - Uses nonce deduplication to prevent double-assignment

## Usage Example

```c
#include "core/node.h"
#include "core/bus_interface.h"
#include "core/hal.h"

// Initialize platform
hal_init();
bus_global_init(4);

// Create node
Bus* bus;
bus_create(&bus, 0, rx_pin, tx_pin);

Node node;
node_init(&node, bus, 0);
node_begin(&node);  // Blocking coordinator election

// Main loop
while (1) {
    node_service(&node);  // Non-blocking
    hal_delay(50);
}
```

## Design Principles

- **Platform Agnostic**: Core logic works unchanged on Arduino and desktop
- **Resource Efficient**: Designed for microcontrollers with limited memory
- **Self-Organizing**: No central configuration required
- **Robust**: Simple protocol with error detection and retry logic
