CC := cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS := -lpthread

SRC := main.c bus.c node.c
HDR := proto.h bus.h node.h
OBJ := $(SRC:.c=.o)
BIN := sim

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

run: $(BIN)
	./$(BIN) 3
