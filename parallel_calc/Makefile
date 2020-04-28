CC = cc
CFLAGS = -std=c99 -pthread 

.PHONY: all clean

all: calculus

debug: calculus.c
	$(CC) $(CFLAGS) -DDEBUG -o $@ calculus.c -lm

calculus: calculus.c
	$(CC) $(CFLAGS) -o $@ calculus.c -lm

clean: 
	rm -f debug calculus
