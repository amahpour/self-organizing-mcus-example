CC := cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -pedantic
SRC := main.c ../shared/node.c ../shared/bus_sim.c ../shared/hal_sim.c
OBJ := $(SRC:.c=.o)
BIN := sim

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c ../shared/node.h ../shared/bus.h ../shared/hal.h ../shared/proto.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

run: $(BIN)
	./$(BIN) 3
