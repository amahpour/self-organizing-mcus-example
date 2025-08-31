# Self-Organizing MCUs - Multi-Platform Build System
# Usage:
#   make sim          - Build PC simulation  
#   make arduino      - Compile Arduino sketch
#   make clean        - Clean all targets
#   make test         - Run simulation tests

.PHONY: all sim arduino arduino-uno arduino-r4-wifi arduino-all clean test help

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
ARDUINO_UNO_FQBN := arduino:avr:uno
ARDUINO_UNO_R4_WIFI_FQBN := arduino:renesas_uno:unor4wifi

# Default Arduino target (classic Uno)
arduino: arduino-uno
	@echo "✅ Arduino sketch compiled for Uno (default)"

# Compile for all Arduino variants
arduino-all: arduino-uno arduino-r4-wifi
	@echo "✅ Arduino sketch compiled for all targets"

# Classic Arduino Uno (AVR)
arduino-uno: $(ARDUINO_SKETCH_DIR)/build-uno
	@echo "✅ Arduino Uno (AVR) compiled successfully"

# Arduino Uno R4 WiFi (Renesas)  
arduino-r4-wifi: $(ARDUINO_SKETCH_DIR)/build-r4-wifi
	@echo "✅ Arduino Uno R4 WiFi (Renesas) compiled successfully"



$(ARDUINO_SKETCH_DIR)/build-uno: $(CORE_SRCS) shared/platform/arduino/bus_arduino.c shared/platform/arduino/hal_arduino.c
	@echo "Compiling universal AutoSort sketch for Arduino Uno (AVR)..."
	@mkdir -p $(ARDUINO_SKETCH_DIR)
	@cp -r shared $(ARDUINO_SKETCH_DIR)/
	arduino-cli compile --fqbn $(ARDUINO_UNO_FQBN) $(ARDUINO_SKETCH_DIR)/
	@touch $@

$(ARDUINO_SKETCH_DIR)/build-r4-wifi: $(CORE_SRCS) shared/platform/arduino_uno_r4/bus_uno_r4.c shared/platform/arduino/hal_arduino.c
	@echo "Compiling universal AutoSort sketch for Arduino Uno R4 WiFi (Renesas)..."
	@mkdir -p $(ARDUINO_SKETCH_DIR)
	@cp -r shared $(ARDUINO_SKETCH_DIR)/
	arduino-cli compile --fqbn $(ARDUINO_UNO_R4_WIFI_FQBN) $(ARDUINO_SKETCH_DIR)/
	@touch $@



# Test targets
test: sim
	@echo "Running simulation tests..."
	./sim/sim 1 && echo "✅ Single node test passed"
	./sim/sim 3 && echo "✅ Multi-node test passed"
	./sim/sim 5 && echo "✅ Stress test passed"

# Clean targets
clean:
	rm -f sim/sim
	rm -rf $(ARDUINO_SKETCH_DIR)/build*
	rm -rf $(ARDUINO_SKETCH_DIR)/shared

	@echo "🧹 Cleaned all build artifacts"

# Format and lint targets
format:
	@echo "Formatting C source files..."
	@find shared/ sim/ -name "*.c" -o -name "*.h" | xargs clang-format -i
	@echo "✅ Code formatting complete"

lint:
	@echo "Linting C source files..."
	@find shared/ sim/ -name "*.c" | xargs clang --analyze -Xanalyzer -analyzer-output=text
	@echo "✅ Static analysis complete"

# Help target
help:
	@echo "Self-Organizing MCUs Build System"
	@echo ""
	@echo "Targets:"
	@echo "  sim              - Build PC simulation (default)"
	@echo "  arduino          - Compile Arduino sketch (Uno classic)"
	@echo "  arduino-uno      - Compile for Arduino Uno (AVR)"
	@echo "  arduino-r4-wifi  - Compile for Arduino Uno R4 WiFi (Renesas)"

	@echo "  arduino-all      - Compile for all Arduino variants"
	@echo "  test             - Run simulation tests"
	@echo "  format           - Format all C source files"
	@echo "  lint             - Run static analysis on C files"
	@echo "  clean            - Clean all build artifacts"
	@echo "  help             - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make sim && ./sim/sim 3"
	@echo "  make arduino-all       # Compile for all Arduino variants"
	@echo "  make arduino-r4-wifi   # Compile specifically for R4 WiFi"

	@echo "  make test"
	@echo "  make format"
