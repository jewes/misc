CC=gcc
all: client server
client: client.c
	$(CC) -o client client.c
server: server.c
	$(CC) -o server server.c

.PHONY: clean
clean:
	rm -f client server
