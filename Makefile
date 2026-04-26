CC ?= clang
LUA ?= luajit

CFLAGS = -std=c23 -Wall -Wextra -Wpedantic
CFLAGS += $(shell pkg-config --cflags --libs $(LUA) fcgi)

BIN = lhp

SOCKET_DIR = /run/lhp
SOCKET := $(SOCKET_DIR)/lhp.sock
GROUP = http

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

socket:
	sudo mkdir -p $(SOCKET_DIR)
	sudo chown $(USER):$(GROUP) $(SOCKET_DIR)
	chmod 775 $(SOCKET_DIR)

run: all socket
	rm -f $(SOCKET)
	spawn-fcgi -s $(SOCKET) -M 660 -G $(GROUP) -- ./$(BIN)

stop:
	pkill $(BIN) || true
	rm -f $(SOCKET)
	
