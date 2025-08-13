# Letting Your Boards Sort Themselves Out

**Goal:** Build a system where identical microcontrollers organize themselves at power-on.

- **Coordinator auto-election**: one board becomes the coordinator automatically
- **Zero pre-config**: no pre-programmed IDs, no jumpers, no special assembly
- **Self-assignment**: new boards announce themselves and get a unique ID from the coordinator

## Architecture Overview

- **Shared bus**: everyone can "hear" broadcasts
- **Roles**:
  - **Coordinator** (1 per network): assigns IDs, optional heartbeats
  - **Member**: requests an ID, does work after joining
- **Startup flow**:
  1. First board up: announces → becomes coordinator
  2. Later boards: announce → coordinator assigns ID
- **Tie-break**: if two claim at once, higher random nonce wins

---

## Project Structure

This project demonstrates **maximum code reuse** between simulation and hardware with **zero ifdefs** in business logic:

```
shared/
├── core/                    # Platform-agnostic business logic (ZERO ifdefs!)
│   ├── node.c              # State machine: SEEKING → COORDINATOR/MEMBER
│   ├── proto.c             # Message framing and validation
│   ├── bus_interface.h     # Abstract bus API
│   └── hal.h               # Hardware abstraction layer
└── platform/               # Platform-specific implementations
    ├── sim/                # Linux simulation (pthreads + queues)
    │   ├── bus_sim.c       # In-process broadcast bus
    │   └── hal_sim.c       # Unix timing + stdio logging
    └── arduino/            # Arduino hardware (UART + SoftwareSerial)
        ├── bus_arduino.c   # UART framing over SoftwareSerial
        └── hal_arduino.c   # Arduino timing + Serial logging
```

### Key Benefits
- **95% code reuse** between simulation and Arduino
- **Zero ifdefs** in shared business logic
- **Clean separation** of platform vs core concerns
- **Easy to extend** - just add new platform folders

---

## Quick Start

### Prerequisites
- **For simulation**: GCC and Make
- **For Arduino**: arduino-cli installed

### Build & Test

```bash
# Build and test simulation
make test                    # Runs 1, 3, and 5 node tests

# Compile Arduino sketch  
make arduino                 # Uses arduino-cli

# Code quality tools
make format                  # Format all C files with clang-format
make lint                    # Run static analysis with clang

# Clean all build artifacts
make clean

# Help
make help
```

### Expected Simulation Output
```
$ make test
Starting simulation with 3 nodes...
Node[0] CLAIM nonce=2494565990
HELLO
JOIN (nonce=257216557)
HELLO  
JOIN (nonce=2201941673)
Node[0] → COORDINATOR (ID=1)
ASSIGN → id=2
ASSIGN received → MEMBER (ID=2)
ASSIGN → id=3
ASSIGN received → MEMBER (ID=3)
✅ All tests passed
```

---

## Hardware Demo (2× Arduino)

### Wiring
- **Power**: Both boards need power (USB or external)
- **Ground**: GND ↔ GND (common ground)
- **Communication**: A.TX → B.RX, B.TX → A.RX (crossover UART)
- **Pins**: Uses pins 10 (RX) and 11 (TX) by default

### Deployment
```bash
# Compile sketch
make arduino

# Flash to both boards (same sketch!)
# Board A: NODE_INDEX = 0 (in AutoSort.ino)  
# Board B: NODE_INDEX = 1 (for startup delay)

# Monitor serial at 115200 baud
# Power A first, then B → A becomes coordinator, B gets ID=2
# Power B first, then A → B becomes coordinator, A gets ID=2
```

### Expected Arduino Output
```
AutoSort Arduino starting...
Node[0] CLAIM nonce=1234567890
Node[0] → COORDINATOR (ID=1)
Node initialized and started
```

---

## Understanding the Code

The codebase is extensively documented with function-level docstrings explaining
the purpose, parameters, and behavior of each component. Key files to read:

### Core State Machine (`shared/core/node.c`)
1. **SEEKING**: Listen for CLAIM messages, if none heard, claim coordinator
2. **COORDINATOR**: Assign IDs to joining members, handle JOIN requests  
3. **MEMBER**: Send periodic JOIN until assigned, then serve requests

### Protocol (`shared/core/proto.c`)
- **HELLO**: Member announces presence
- **CLAIM**: Node claims coordinator with random nonce (tie-break)
- **JOIN**: Member requests ID assignment  
- **ASSIGN**: Coordinator assigns unique ID

### Platform Abstraction
- **HAL** (`hal.h`): `hal_millis()`, `hal_delay()`, `hal_random32()`, `hal_log()`
- **Bus** (`bus_interface.h`): `bus_send()`, `bus_recv()` with timeout

### Code Quality
- **Comprehensive docstrings**: Every function has detailed documentation
- **Consistent formatting**: Code styled with clang-format for readability
- **Student-friendly**: Complex algorithms broken down with inline comments

---

## Extending to New Platforms

Adding ESP32, STM32, or other platforms is straightforward:

1. **Create platform directory**:
   ```
   shared/platform/esp32/
   ├── bus_esp32.c     # WiFi mesh or Bluetooth
   └── hal_esp32.c     # ESP-IDF timing
   ```

2. **Implement the interfaces** defined in `hal.h` and `bus_interface.h`

3. **Add Makefile target** for the new platform

4. **No changes needed** to core business logic!

---

## Learning Objectives

This project teaches:
- **Distributed systems**: leader election, consensus, message passing
- **Embedded programming**: UART communication, timing, state machines  
- **Software architecture**: abstraction layers, interface design
- **Code reuse**: platform abstraction without complexity

Perfect for students learning embedded systems and distributed algorithms!

## Next Steps (Optional Extensions)

- **Heartbeats**: Coordinator broadcasts every 1s; members track connectivity
- **Re-election**: If coordinator fails, members re-run election
- **RS-485**: Multi-drop bus supporting many boards on 2 wires
- **Mesh networking**: Multi-hop routing between coordinator islands