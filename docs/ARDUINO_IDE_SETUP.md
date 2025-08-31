# Arduino IDE Setup Guide

This guide shows how to manually compile and upload the AutoSort project using the Arduino IDE.

## Prerequisites

- Arduino IDE 1.8.x or 2.x installed
- Two Arduino boards (Uno, Nano, etc.)
- USB cables for both boards

## Project Structure Note

This project uses a shared codebase architecture where the Arduino sketch includes C files from the `shared/` directory. The Arduino IDE needs to be able to find these files.

## Setup Steps

### 1. Prepare the Shared Files (Critical!)
The Arduino IDE needs the shared source files to be copied into the sketch directory:

```bash
# From the project root directory:
cp -r shared arduino/AutoSort/
```

**Why this is needed:** The Arduino IDE looks for included files relative to the sketch directory, but this project keeps shared code in a separate folder for reuse between simulation and hardware.

### 2. Open the Sketch
1. Launch Arduino IDE
2. Open `arduino/AutoSort/AutoSort.ino`
3. The IDE should now be able to find all the included files

### 3. Verify File Structure
In the Arduino IDE, you should see these tabs:
- `AutoSort.ino` (main sketch)
- The IDE may show additional tabs for included `.c` files

You can also verify the files exist in your file system:
```
arduino/AutoSort/
├── AutoSort.ino
└── shared/
    ├── core/
    │   ├── node.c, node.h
    │   ├── proto.c, proto.h
    │   ├── bus_interface.h
    │   └── hal.h
    └── platform/
        └── arduino/
            ├── bus_arduino.c
            └── hal_arduino.c
```

### 4. Node Configuration
The sketch uses the same code for both boards:
```cpp
// Line 33 in AutoSort.ino - same for both boards:
node_init(&node, bus, 0);
```

**Important:** You upload the **identical sketch** to both boards. The coordinator election happens based on **power-on timing**, not code differences.

### 5. Board Configuration
1. **Tools → Board** → Select your Arduino model (e.g., "Arduino Uno")
2. **Tools → Port** → Select the COM port for your first Arduino

### 6. Compile and Upload Board A
1. Click **Verify** (✓) to compile - should show "Done compiling"
2. Click **Upload** (→) to flash Board A
3. Open **Tools → Serial Monitor** 
4. Set baud rate to **115200**
5. You should see: `AutoSort Arduino starting...`

### 7. Upload to Board B
1. **Tools → Port** → Select Board B's port
2. Click **Upload** to flash the **same identical sketch** to Board B
3. No code changes needed - both boards run identical code!

### 8. Hardware Wiring
Connect the boards as shown in the wiring diagram:

```
Arduino A          Arduino B
TX (Pin 11) -----> RX (Pin 10)
RX (Pin 10) <----- TX (Pin 11)  
GND         -----> GND
```

### 9. Test the Demo
1. **Power both boards** via USB
2. Open Serial Monitor for both boards (separate IDE windows)
3. **Reset Board A first** - should become coordinator
4. **Reset Board B** - should join as member

## Expected Output

**Board A (Coordinator):**
```
AutoSort Arduino starting...
Node[0] CLAIM nonce=1234567890
Node[0] → COORDINATOR (ID=1)
ASSIGN → id=2
```

**Board B (Member):**
```
AutoSort Arduino starting...
HELLO
JOIN (nonce=5678901234)
ASSIGN received → MEMBER (ID=2)
```

## Troubleshooting

### Compilation Errors
- **"No such file or directory"**: Make sure you opened `AutoSort.ino` from the correct path
- **C++ template errors**: Try using Arduino IDE 1.8.x if using 2.x causes issues
- **SoftwareSerial issues**: Some newer Arduino models may have compatibility issues

### Upload Issues
- **Port not found**: Check USB cable and drivers
- **Permission denied**: Try different USB port or restart IDE
- **Board not responding**: Press reset button and try upload again

### Communication Issues
- **No serial output**: Check baud rate is 115200
- **Boards not talking**: Verify crossover wiring (TX→RX, RX→TX)
- **Both become coordinators**: Check node index values are different

## Alternative: Using arduino-cli

For a simpler workflow, consider using the provided Makefile:
```bash
make arduino    # Compile sketch
# Then use IDE just for uploading
```

## How Coordinator Election Works

The system uses **power-on timing** for coordinator election:
- **First board powered**: Listens for other coordinators, finds none, becomes coordinator
- **Second board powered**: Finds existing coordinator, joins as member
- **Identical code**: Both boards run the exact same sketch - no hardcoded differences!

The `node_init(&node, bus, 0)` parameter could be used for startup delays, but in this demo we rely on natural power-on timing differences.

## Next Steps

Once you have the basic two-board demo working:
1. Try power sequencing (reset boards in different orders)
2. Experiment with 3+ boards
3. Monitor the coordinator election process
4. Explore the MCP demo tools for advanced testing
