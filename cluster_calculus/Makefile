CC=gcc
CFLAGS=-std=gnu99
LDFLAGS=-pthread -lm
BIN_DIR=bin/

WORKER=worker
SERVER=server

.PHONY: all clean

all: mkbindir $(WORKER) $(SERVER)

clean:
	rm -r bin

mkbindir:
	mkdir -p bin

$(WORKER) : paralleling_api.o worker.o lib.o
	cd bin; $(CC) $^ -o $@ $(LDFLAGS);

$(SERVER) : server.o lib.o
	cd bin; $(CC) $^ -o $@ $(LDFLAGS);

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $(BIN_DIR)$@
