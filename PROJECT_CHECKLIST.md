# Project Checklist

Use this as an ordered set of tasks. Each line can map to a GitHub issue or Cursor task.

## Phase 1 — Local Simulation (PC)

- [ ] **P1-01: Define protocol**
  - Create `sim/proto.h` with enums for `HELLO`, `CLAIM`, `JOIN`, `ASSIGN`
  - Define a compact `Frame` struct: start byte, type, source, payload (optional), checksum

- [ ] **P1-02: In-process bus**
  - Create `sim/bus.h`, `sim/bus.c`
  - Implement a broadcast bus shared by all nodes (e.g., thread-safe queue per node)
  - API: `bus_init(n)`, `bus_subscribe(id)`, `bus_broadcast(frame)`, `bus_recv(id, frame, timeout_ms)`

- [ ] **P1-03: Node state machine**
  - Create `sim/node.h`, `sim/node.c`
  - States: `SEEKING`, `COORDINATOR`, `MEMBER`
  - Behavior:
    - On startup, wait short jitter; listen for `CLAIM`
    - If none heard, send `CLAIM` with random nonce; become `COORDINATOR` unless higher nonce observed
    - Members send `HELLO`/`JOIN`; coordinator replies `ASSIGN` with next ID

- [ ] **P1-04: Main driver**
  - Create `sim/main.c` to spawn N nodes (threads)
  - Add small staggered delays (e.g., 0/150/300 ms) for deterministic demos
  - Print concise events: CLAIM, JOIN, ASSIGN, role transitions

- [ ] **P1-05: Makefile**
  - Create `sim/Makefile` to build all objects and link to `./sim`
  - Targets: `all`, `run`, `clean`

- [ ] **P1-06: Single-node test**
  - Run `./sim 1` → node becomes coordinator with ID=1

- [ ] **P1-07: Multi-node test**
  - Run `./sim 3` → IDs assigned 1..N with one coordinator

- [ ] **P1-08: Tie-break test (bonus)**
  - Launch two nodes with identical startup delay; ensure only one remains coordinator (higher nonce wins)

- [ ] **P1-09: Stability pass**
  - Re-run multiple times; verify no deadlocks and no duplicate IDs

## Phase 2 — Two Arduinos (UART)

- [ ] **P2-01: Arduino scaffold**
  - Create `arduino/` folder with `AutoSort.ino`

- [ ] **P2-02: Protocol header**
  - Port `Proto.h` with start byte and checksum (simple XOR or sum)

- [ ] **P2-03: UART bus**
  - Implement `Bus.h/.cpp` using SoftwareSerial (TX↔RX + GND)
  - API: `begin(baud)`, `send(const Frame&)`, `bool recv(Frame&, uint16_t timeout_ms)`

- [ ] **P2-04: Node logic**
  - Port state machine to `Node.h/.cpp` with same CLAIM/JOIN/ASSIGN flow and nonce tie-break

- [ ] **P2-05: Sketch glue**
  - In `AutoSort.ino`, `setup()` initializes Serial and bus; `loop()` services node and prints key events

- [ ] **P2-06: Two-board demo**
  - Flash same sketch to both boards
  - Test power order A→B and B→A; check logs for roles and IDs

- [ ] **P2-07: Cleanup and docs**
  - Ensure concise Serial logs; document wiring diagram and steps in README

### Optional extensions
- [ ] **HB-01: Heartbeats** — coordinator broadcasts every 1s; members track last-seen
- [ ] **HB-02: Re-election** — if no heartbeat for ~3s, reopen election window
- [ ] **RS485-01: MAX485 bus** — replace SoftwareSerial wiring with RS-485 modules
- [ ] **I2C-01: Single-master variant** — coordinator as I²C master; members as slaves after assignment
