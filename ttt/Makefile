CC = gcc
CFLAGS = -Wall -Wextra -O2

all: client server

client: ttt.c protocol.c protocol.h
	$(CC) $(CFLAGS) -o client ttt.c protocol.c

server: ttts.c protocol.c protocol.h
	$(CC) $(CFLAGS) -o server ttts.c protocol.c

clean:
	rm -f client server
