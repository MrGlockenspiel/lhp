CC ?= clang
LUA ?= luajit

CFLAGS = -std=c23 -Wall -Wextra -Wpedantic
CFLAGS += $(shell pkg-config --cflags --libs $(LUA) fcgi)

BIN = lhp
PORT = 9000

ifdef RELEASE
CFLAGS += -O3
else
CFLAGS += -Og -g
endif

.PHONY: all clean run stop

all:
	$(CC) $(CFLAGS) $(wildcard src/*.c) -o $(BIN)

clean:
	rm -f $(BIN)

run: all
	spawn-fcgi -p $(PORT) -- $(BIN)

stop:
	pkill $(BIN)
