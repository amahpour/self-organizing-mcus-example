# Developer Guide

## Architecture

- **Core logic** (`shared/core/`) - Platform-agnostic, no ifdefs
- **Platform implementations** (`shared/platform/`) - HAL and bus interfaces  
- **95% code reuse** between simulation and hardware

## Adding New Platforms

To add ESP32, STM32, Raspberry Pi, etc:

1. Create `shared/platform/myplatform/`
2. Implement `hal_myplatform.c` (timing, random, logging)
3. Implement `bus_myplatform.c` (communication layer)
4. Add build target to `Makefile`

The core logic stays unchanged.

## Key Rules

- **No ifdefs in core logic** - use HAL abstraction instead
- **Minimal interfaces** - keep platform contracts simple  
- **Test on real hardware** - simulation can't catch all timing issues

## File Structure

```
shared/core/           # Platform-agnostic business logic
shared/platform/X/     # Platform X implementations  
sim/                   # PC simulation
arduino/               # Arduino hardware
```
