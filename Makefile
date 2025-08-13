# Self-Organizing MCUs - Multi-Platform Build System
# Usage:
#   make sim          - Build PC simulation  
#   make arduino      - Compile Arduino sketch
#   make clean        - Clean all targets
#   make test         - Run simulation tests

.PHONY: all sim arduino clean test help

# Default target
all: sim

# Core sources (platform-agnostic business logic)
CORE_SRCS := shared/core/proto.c shared/core/node.c

# Simulation build
SIM_SRCS := $(CORE_SRCS) shared/platform/sim/bus_sim.c shared/platform/sim/hal_sim.c sim/main.c
SIM_CC := cc
SIM_CFLAGS := -std=c11 -O2 -Wall -Wextra -pedantic -Ishared/core -Ishared/platform/sim
SIM_LDFLAGS := -lpthread

sim: sim/sim
	@echo "✅ Simulation built successfully"

sim/sim: $(SIM_SRCS)
	$(SIM_CC) $(SIM_CFLAGS) -o $@ $^ $(SIM_LDFLAGS)

# Arduino build (uses arduino-cli)
ARDUINO_SKETCH_DIR := arduino/AutoSort
ARDUINO_FQBN := arduino:avr:uno

arduino: $(ARDUINO_SKETCH_DIR)/build
	@echo "✅ Arduino sketch compiled successfully"

$(ARDUINO_SKETCH_DIR)/build: $(CORE_SRCS) shared/platform/arduino/bus_arduino.c shared/platform/arduino/hal_arduino.c
	@echo "Preparing Arduino sketch..."
	@mkdir -p $(ARDUINO_SKETCH_DIR)
	@cp -r shared $(ARDUINO_SKETCH_DIR)/
	@echo "Compiling Arduino sketch..."
	arduino-cli compile --fqbn $(ARDUINO_FQBN) $(ARDUINO_SKETCH_DIR)/
	@touch $@  # Mark build complete

# Test targets
test: sim
	@echo "Running simulation tests..."
	./sim/sim 1 && echo "✅ Single node test passed"
	./sim/sim 3 && echo "✅ Multi-node test passed"
	./sim/sim 5 && echo "✅ Stress test passed"

# Clean targets
clean:
	rm -f sim/sim
	rm -rf $(ARDUINO_SKETCH_DIR)/build
	rm -rf $(ARDUINO_SKETCH_DIR)/shared
	@echo "🧹 Cleaned all build artifacts"

# Help target
help:
	@echo "Self-Organizing MCUs Build System"
	@echo ""
	@echo "Targets:"
	@echo "  sim      - Build PC simulation (default)"
	@echo "  arduino  - Compile Arduino sketch"
	@echo "  test     - Run simulation tests"
	@echo "  clean    - Clean all build artifacts"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make sim && ./sim/sim 3"
	@echo "  make arduino"
	@echo "  make test"
