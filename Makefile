CC ?= clang
LUA ?= luajit

CFLAGS = -std=c23 -Wall -Wextra -Wpedantic
LDFLAGS = 

CFLAGS += $(shell pkg-config --cflags $(LUA) fcgi)
LDFLAGS += $(shell pkg-config --libs $(LUA) fcgi)

BIN = lhp
PORT = 9000

ifdef RELEASE
CFLAGS += -O3
else
CFLAGS += -Og -g
endif

SRC = $(wildcard src/*.c)

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(BIN)

run: all
	spawn-fcgi -p $(PORT) -- $(BIN)

stop:
	pkill $(BIN)
