CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_XOPEN_SOURCE=700
SERVER_DEPS = tttsss.c protocol.c protocol.h uthash.h

all: client server

client: ttt.c protocol.c protocol.h
	$(CC) $(CFLAGS) -o client ttt.c protocol.c -lpthread

server: $(SERVER_DEPS)
	$(CC) $(CFLAGS) -o server tttsss.c protocol.c -lpthread

clean:
	rm -f client server
