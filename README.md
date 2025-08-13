# Letting Your Boards Sort Themselves Out

**Goal:** identical boards that auto-elect a coordinator and get unique IDs at power-on.

## Scope
- **In**: Basic leader election (first/strongest becomes coordinator), ID assignment, minimal message passing
- **Out (for now)**: Safety certification, multi-hop routing, fault-tolerant consensus, cryptography

## Audience assumptions
- Comfortable with C basics and Arduino sketches
- New to distributed/leader-election ideas
- Prefer simple wiring and minimal moving parts

## Architecture in 60 seconds
- **Shared bus**: everyone can “hear” broadcasts
- **Roles**:
  - Coordinator (1 per network): assigns IDs, optional heartbeats
  - Member: requests an ID, does work after join
- **Startup flow**:
  1. First board up: announces → becomes coordinator
  2. Later boards: announce → coordinator assigns ID
- **Tie-break (optional)**: if two claim at once, higher random nonce wins

---

## Phase 1 — Local Simulation (PC)

### Objective
Prove the idea without hardware by running several “virtual boards” on your computer and a simple “bus” so they can talk.

### What we’ll build
- Minimal protocol: `HELLO`, `CLAIM`, `JOIN`, `ASSIGN`
- In-process bus: a tiny broadcast mechanism (threads + queues)
- Node state machine: `SEEKING → COORDINATOR` or `MEMBER`

### Files to create
- `sim/proto.h` – message types + small frame struct
- `sim/bus.h`, `sim/bus.c` – a simple broadcast bus the threads share
- `sim/node.h`, `sim/node.c` – the node state machine (startup, claim/join, assign IDs)
- `sim/main.c` – spawns N nodes with small startup delays; prints events
- `sim/Makefile` – build + run

### Milestones
1. One node: starts, becomes coordinator (ID=1)
2. Two nodes: second joins and gets ID=2)
3. Three+ nodes: all get unique IDs in order
4. Tie-break (bonus): start two at once; only one wins coordinator

### Definition of done (Phase 1)
- Running `./sim 3` shows one coordinator (ID=1) and members with IDs 2..N
- Repeated runs yield stable results (no deadlocks, no duplicates)
- Output is readable and minimal (good for learning)

---

## Phase 2 — Two Arduinos (real hardware)

### Objective
Show the same behavior with two identical Arduino boards using a simple UART link (TX↔RX + GND).

### Why UART (not I²C)?
- UART is simpler and more forgiving for absolute beginners
- Arduino’s I²C multi-master is fussy; we can revisit later

### Hardware
- 2× Arduino Uno (or similar)
- Wires: GND↔GND, A.TX→B.RX, B.TX→A.RX (e.g., SoftwareSerial pins)
- Keep USB free for debug prints

### What we’ll write
- `arduino/Proto.h`: same message types, tiny frame with checksum
- `arduino/Bus.h`, `arduino/Bus.cpp`: `begin()`, `send(frame)`, `recv(frame)` using SoftwareSerial
- `arduino/Node.h`, `arduino/Node.cpp`: same startup logic as sim
- `arduino/AutoSort.ino`: `setup()` = bus + node init; `loop()` = node service + Serial prints

### Milestones
1. Single board: powers up and declares itself coordinator (prints message)
2. Second board added: announces, receives ASSIGN (prints “My ID is 2”)
3. Swap power order: the first up becomes coordinator (behavior flips correctly)

### Definition of done (Phase 2)
- Same sketch on both boards
- Power order decides the coordinator
- Clean, readable serial logs confirm IDs

---

## Testing checklist
- Cold start: Power A, then B → A becomes coordinator, B gets ID=2
- Reverse: Power B, then A → B becomes coordinator, A gets ID=2
- Sim burst: In Phase 1, start three threads with 0/150/300 ms delays → IDs 1,2,3
- Tie-break (optional): Launch two nodes with identical delays; only one claims coordinator

## Nice-to-haves
- Heartbeat: Coordinator broadcasts every 1s; members log “last seen.”
- Re-election: If a member misses heartbeats for ~3s, rerun the election window
- RS-485 upgrade: Swap UART wires for MAX485 modules to support many boards on a 2-wire bus
- I²C (single-master): Coordinator as I²C master, members as slaves after they get IDs

## Common pitfalls
- Bus contention/collisions: Keep it simple with broadcast + short listen windows; UART is point-to-point in the 2-board demo
- Ambiguous frames: Use a start byte and a checksum; keep payloads tiny
- Both think they’re boss: Use a random nonce; highest wins
- Complex logging: Print only key events (CLAIM, JOIN, ASSIGN) so learners aren’t overwhelmed

---

## Quickstart

### Phase 1 (PC sim)
- Build: `cd sim && make`
- Run: `./sim 3`
- Expect: one coordinator (ID=1), others get IDs 2..N

### Phase 2 (2× Arduino over UART)
- Wire GND↔GND, A.TX→B.RX, B.TX→A.RX
- Flash the same sketch to both
- Power in different orders; watch IDs/roles via Serial

### Next steps (optional)
- Heartbeats + re-election
- RS-485 multi-drop bus
- I²C (single-master) variant
