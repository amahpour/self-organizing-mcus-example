# Development Guide

This guide is for contributors who want to understand or extend the codebase.

## Architecture Principles

1. **Zero ifdefs in business logic** - platform detection happens at build time, not runtime
2. **Clean interfaces** - `hal.h` and `bus_interface.h` define platform contracts
3. **Single source of truth** - core logic in `shared/core/` is platform-agnostic
4. **Simple build system** - Make for sim, arduino-cli for hardware

## Adding a New Platform

Want to add ESP32, STM32, Raspberry Pi, etc.? Here's how:

### 1. Create Platform Directory
```bash
mkdir -p shared/platform/myplatform
```

### 2. Implement HAL Interface
Create `shared/platform/myplatform/hal_myplatform.c`:
```c
#include "../../core/hal.h"

void hal_init(void) {
    // Platform-specific initialization
}

uint32_t hal_millis(void) {
    // Return milliseconds since boot
}

// ... implement other hal.h functions
```

### 3. Implement Bus Interface  
Create `shared/platform/myplatform/bus_myplatform.c`:
```c
#include "../../core/bus_interface.h"

struct Bus {
    // Platform-specific bus state
};

int bus_create(Bus** bus, uint8_t node_index, uint8_t rx_pin, uint8_t tx_pin) {
    // Create and initialize bus
}

// ... implement other bus_interface.h functions
```

### 4. Add Build Target
Add to root `Makefile`:
```makefile
ifeq ($(TARGET),myplatform)
    PLATFORM_SRCS = shared/platform/myplatform/bus_myplatform.c shared/platform/myplatform/hal_myplatform.c
    # Platform-specific compiler/linker settings
endif
```

### 5. Test
```bash
make TARGET=myplatform
```

## Code Style

- **C11 standard** for maximum compatibility
- **Snake_case** for functions and variables  
- **PascalCase** for types (structs, enums)
- **SCREAMING_CASE** for constants and macros
- **Minimal dependencies** - prefer standard library

## Testing Strategy

- **Unit tests**: Each platform implementation should be testable in isolation
- **Integration tests**: `make test` runs multi-node scenarios
- **Hardware tests**: Manual verification on real boards

## Common Pitfalls

1. **Don't add ifdefs to core logic** - use the HAL instead
2. **Keep interfaces minimal** - resist adding platform-specific parameters
3. **Test on real hardware** - simulation can't catch all timing issues
4. **Document platform quirks** - note any limitations or special requirements

## File Organization

```
shared/core/           # Never has platform ifdefs
shared/platform/X/     # Platform X implementation
sim/                   # Simulation entry point
arduino/               # Arduino entry point (gets shared code copied)
```

## Build System Notes

- **Simulation**: Standard Make with GCC
- **Arduino**: arduino-cli with copied source files (Arduino IDE limitation)
- **Future platforms**: Can use CMake, Bazel, or platform-specific tools

The key is keeping the core logic (`shared/core/`) buildable by any system.
