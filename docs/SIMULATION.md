# Simulation Overview

The `sim` directory contains a multi-threaded simulation that tests the self-organizing microcontroller system on a PC before deploying to actual Arduino hardware. The simulation validates the distributed coordinator election algorithm and member management protocol.

## What It Does

The simulation creates a configurable number of virtual nodes (1-16, default 3) that communicate through an in-process message bus. Each node runs in its own pthread thread and follows the same state machine as the real Arduino code:

1. **Coordinator Election**: Nodes start in `SEEKING` state and compete to become the `COORDINATOR` using random nonces for tie-breaking
2. **Member Management**: Non-coordinator nodes become `MEMBER` nodes and request unique ID assignments  
3. **Communication Protocol**: All nodes exchange messages (`HELLO`, `CLAIM`, `JOIN`, `ASSIGN`, `HEARTBEAT`) using the same wire protocol as the hardware

## Key Components

- **`sim/main.c`**: Creates threaded nodes and manages the 3-second simulation lifecycle
- **`shared/platform/sim/bus_sim.c`**: Implements broadcast messaging using pthread mutexes and condition variables with ring buffers
- **Shared Core Logic**: Uses the same platform-agnostic `node.c` and `proto.c` code as the Arduino implementation

## Running the Simulation

Build and run the simulation using the Makefile:

```bash
# Build the simulation
make sim

# Run with different node counts
./sim/sim 1   # Single node test
./sim/sim 3   # Default multi-node test  
./sim/sim 5   # Stress test with 5 nodes
```

## Testing

The Makefile includes automated tests that validate different scenarios:

```bash
make test  # Runs three test scenarios:
           # - Single node (becomes coordinator)
           # - Multi-node coordination test  
           # - Stress test with 5 nodes
```

### Test Case Analysis

#### Test Case 1: Single Node Test (`./sim/sim 1`)

**What it tests:**
- Basic coordinator self-election when no other nodes are present
- Node initialization and state machine progression
- Timeout-based coordinator claiming mechanism

**Acceptance Criteria:**
- ✅ Node successfully listens for ~1000ms without hearing claims
- ✅ Node sends CLAIM message with random nonce
- ✅ Node transitions to `COORDINATOR` state with ID=1
- ✅ Process completes without errors or timeouts

#### Test Case 2: Multi-Node Test (`./sim/sim 3`)

**What it tests:**
- Distributed coordinator election with multiple nodes
- Race condition handling during startup
- Member joining and ID assignment process
- Message broadcast and reception

**Acceptance Criteria:**
- ✅ Exactly one node becomes COORDINATOR
- ✅ All other nodes become MEMBER nodes with unique sequential IDs
- ✅ JOIN nonces are properly echoed in ASSIGN responses
- ✅ No duplicate ID assignments occur
- ✅ All nodes reach stable final states

#### Test Case 3: Stress Test (`./sim/sim 5`)

**What it tests:**
- System scalability with higher node count
- Race condition handling under increased contention
- Coordinator's ability to manage multiple simultaneous JOIN requests

**Acceptance Criteria:**
- ✅ Single coordinator elected despite higher contention
- ✅ All four members successfully join with sequential IDs (2, 3, 4, 5)
- ✅ No message loss or corruption under higher traffic
- ✅ System remains stable with increased load

### Critical Properties Verified

1. **Safety**: Never more than one coordinator
2. **Liveness**: All nodes eventually reach stable states
3. **Uniqueness**: No duplicate ID assignments
4. **Reliability**: Message protocol works under various conditions
5. **Scalability**: System functions with 1-5 nodes (supports up to 16)

## Architecture Benefits

The simulation validates the core distributed algorithm before hardware deployment, ensuring:

- Coordinator election logic works correctly with multiple nodes
- Race conditions are handled properly during startup
- Member ID assignments work without conflicts
- Message protocol operates reliably

Since the simulation shares the same core code (`shared/core/`) as the Arduino implementation, it provides high confidence that the hardware version will behave identically.

## Implementation Details

### Threading Model
Each virtual node runs in its own pthread thread with a 10ms service interval, mimicking the timing behavior of the Arduino main loop.

### Message Bus
The simulation uses a broadcast message bus where each node has its own message queue. When a node sends a frame, it's delivered to all node queues simultaneously, simulating a shared communication medium.

### Lifecycle
The simulation runs for 3 seconds, which is sufficient time for coordinator election and member joining to complete, then cleanly shuts down all threads.
