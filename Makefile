CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -std=c11
LDFLAGS ?= -lm

SRC = catan.c catan_map.c catan_menu.c catan_game.c
OBJ = $(SRC:.c=.o)
BIN = catan

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
endif

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN) catan
