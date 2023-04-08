CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_XOPEN_SOURCE=700

all: client server

client: ttt.c protocol.c protocol.h
	$(CC) $(CFLAGS) -o client ttt.c protocol.c -lpthread

server: ttts.c protocol.c protocol.h
	$(CC) $(CFLAGS) -o server ttts.c protocol.c -lpthread

clean:
	rm -f client server
