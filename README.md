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
.
├── DEVELOPMENT.md           # Development notes and internal documentation
├── Makefile                # Build system for simulation and Arduino
├── README.md               # This file
├── arduino/                # Arduino sketch and project files
│   └── AutoSort/           # Main Arduino sketch
│       └── AutoSort.ino    # Arduino implementation using shared code
├── docs/                   # Documentation and guides
│   ├── AGENTIC_HARDWARE_CONTROL.md # AI-driven hardware control guide
│   ├── ARDUINO_IDE_SETUP.md        # Arduino IDE setup instructions
│   ├── DEMO_QUICK_REFERENCE.md     # Quick demo reference
│   ├── DIAGRAMS.md                 # System diagrams and visualizations
│   ├── MCP_DEMO.md                 # MCP server demo guide
│   ├── SIMULATION.md               # Simulation documentation
│   └── images/                     # Documentation images
│       └── startup_sequence.png    # System startup flow diagram
├── shared/                 # Shared code between platforms
│   ├── README.md           # Shared code documentation
│   ├── core/               # Platform-agnostic business logic (ZERO ifdefs!)
│   │   ├── bus_interface.h # Abstract bus API
│   │   ├── hal.h           # Hardware abstraction layer
│   │   ├── node.c          # State machine: SEEKING → COORDINATOR/MEMBER
│   │   ├── node.h          # Node state and function declarations
│   │   ├── proto.c         # Message framing and validation
│   │   └── proto.h         # Protocol message type definitions
│   └── platform/           # Platform-specific implementations
│       ├── arduino/        # Arduino hardware (UART + SoftwareSerial)
│       │   ├── bus_arduino.c # UART framing over SoftwareSerial
│       │   └── hal_arduino.c # Arduino timing + Serial logging
│       ├── arduino_uno_r4/ # Arduino UNO R4 specific implementation
│       │   └── bus_uno_r4.c # UNO R4 optimized bus implementation
│       └── sim/            # Linux simulation (pthreads + queues)
│           ├── bus_sim.c   # In-process broadcast bus
│           └── hal_sim.c   # Unix timing + stdio logging
└── sim/                    # Simulation executable and test files
    └── main.c              # Simulation entry point and test harness
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

# Setup Arduino CLI and board packages
make setup-arduino-cli       # Initialize Arduino CLI
make setup-minicore          # Install MiniCore for ATmega328P

# Compile Arduino sketch  
make arduino                 # Uses arduino-cli (default: Arduino Uno)
make arduino-atmega328p      # Compile for bare ATmega328P (MiniCore)
make arduino-r4-wifi         # Compile for Arduino Uno R4 WiFi
make arduino-all             # Compile for all supported platforms

# Program ATmega328P
make burn-bootloader-atmega328p  # Set fuses (8MHz, BOD 2.7V)
make program-atmega328p          # Program with AutoSort sketch

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

## Supported Platforms

This project now supports **three Arduino-compatible platforms**:

1. **Arduino Uno (Classic)** - 16MHz external crystal, 5V logic
2. **Arduino Uno R4 WiFi** - Renesas RA4M1, hardware Serial1
3. **Bare ATmega328P** - 8MHz internal RC, 3.3V logic, Atmel-ICE programming

All platforms use the **same AutoSort sketch** with compile-time board detection.

---

## Hardware Demo (2× Arduino)

### Wiring
- **Power**: Both boards need power (USB or external)
- **Ground**: GND ↔ GND (common ground)
- **Communication**: A.TX → B.RX, B.TX → A.RX (crossover UART)
- **Pins**: Uses pins 10 (RX) and 11 (TX) by default

### Key Concept: Identical Code + Power Sequencing
- **Same sketch** flashed to both boards - no code differences!
- **Timing determines roles**: First board to power on becomes coordinator
- **Power sequencing demo**: Reset boards in different orders to see role switching
- **No manual configuration**: The boards figure out their roles automatically

### Deployment Options

**Option 1: Command Line (Recommended)**
```bash
# Compile sketch
make arduino

# Flash identical sketch to both boards
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:uno arduino/AutoSort
arduino-cli upload -p /dev/ttyUSB1 --fqbn arduino:avr:uno arduino/AutoSort

# Monitor serial at 115200 baud and use power sequencing:
# Power A first, then B → A becomes coordinator, B gets ID=2
# Reset both, power B first → B becomes coordinator, A gets ID=2
```

**Option 2: Arduino IDE**  
📖 **[Arduino IDE Setup Guide](docs/ARDUINO_IDE_SETUP.md)** - Complete manual setup instructions

### Expected Arduino Output
```
AutoSort Arduino starting...
Node[0] CLAIM nonce=1234567890
Node[0] → COORDINATOR (ID=1)
Node initialized and started
```

---

## Bare ATmega328P Demo

### Prerequisites
- **Atmel-ICE** programmer (or compatible ISP programmer)
- **MiniCore** board package for Arduino CLI
- **ATmega328P** chip on breadboard with minimal wiring

### Setup
```bash
# Setup Arduino CLI and MiniCore (one-time setup)
make setup-minicore

# Compile for ATmega328P
make arduino-atmega328p
```

### Wiring (ATmega328P ↔ Atmel-ICE)
| Atmel-ICE AVR pin | ATmega328P DIP pin |
|-------------------|-------------------|
| 1 (SCK)           | 19 (PB5 / SCK)    |
| 2 (GND)           | 8 or 22 (GND)     |
| 3 (MISO)          | 18 (PB4 / MISO)   |
| 4 (VTG)           | 7 (VCC)           |
| 6 (nRESET)        | 1 (PC6 / RESET)   |
| 9 (MOSI)          | 17 (PB3 / MOSI)   |

**Power/decoupling (minimum):**
- VCC (pin 7) → **3.3 V**
- AVCC (pin 20) → **3.3 V** (tie to VCC)
- GND (pins 8/22) → **GND**
- 0.1 µF cap near VCC–GND

### Inter-board Communication (ATmega328P ↔ Arduino Uno)
- **ATmega328P pin 18 (PB4)** → **Arduino Uno pin 10 (RX)**
- **ATmega328P pin 19 (PB5)** → **Arduino Uno pin 11 (TX)**
- **GND** ↔ **GND**
- **3.3V** logic (level shifting may be needed for 5V Arduino)

### Programming & Testing
```bash
# Set fuses for 8MHz internal RC + BOD 2.7V (one-time setup per chip)
make burn-bootloader-atmega328p

# Program the ATmega328P chip with AutoSort sketch
make program-atmega328p

# Test UART communication (38400 baud)
# Use your preferred serial terminal (minicom, screen, etc.)
# Device: /dev/ttyAMA2 (or your UART device)
# Baud: 38400
```

### Key Differences from Arduino Uno
- **Clock**: 8MHz internal RC (vs 16MHz external crystal)
- **Voltage**: 3.3V operation (vs 5V Arduino)
- **Debug Serial**: 38400 baud (vs 115200 Arduino)
- **Bus Communication**: 4800 baud SoftwareSerial (vs 9600 Arduino)
- **Programming**: Atmel-ICE ISP (vs USB bootloader)

---

## MCP Demo (Recommended)

For the **best demonstration experience**, use the [Arduino MCP Server](https://github.com/amahpour/arduino-mcp-server-simple) for seamless development and testing:

📖 **[Complete MCP Demo Guide](docs/MCP_DEMO.md)** - Comprehensive documentation  
🚀 **[Quick Reference](docs/DEMO_QUICK_REFERENCE.md)** - Fast demo steps

### Why Use MCP?
- **Power Sequencing Demo:** Upload sketches to simulate power-on timing
- **Real-time Monitoring:** Live serial output from multiple boards
- **Identical Sketches:** No manual NODE_INDEX changes needed
- **Professional Tools:** Seamless compile/upload/monitor workflow

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

Adding ESP32, STM32, or other platforms is straightforward. This project demonstrates multiple approaches:

### **Approach 1: Separate Platform Implementation**
For completely different architectures (ESP32, STM32):
1. **Create platform directory**:
   ```
   shared/platform/esp32/
   ├── bus_esp32.c     # WiFi mesh or Bluetooth
   └── hal_esp32.c     # ESP-IDF timing
   ```

2. **Implement the interfaces** defined in `hal.h` and `bus_interface.h`

3. **Add Makefile target** for the new platform

### **Approach 2: Arduino Variant (Like ATmega328P)**
For Arduino-compatible chips with different configurations:
1. **Add board detection** in `AutoSort.ino`
2. **Adjust baud rates/clocks** for the specific hardware
3. **Reuse existing HAL and bus implementations**
4. **Add new Makefile target** with appropriate FQBN

### **Key Benefits**
- **95% code reuse** between simulation and all hardware platforms
- **Zero ifdefs** in shared business logic
- **Clean separation** of platform vs core concerns
- **Easy to extend** - just add new platform folders or Arduino variants

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