# Compilador e flags
CC = gcc
CFLAGS = -Wall -Iinclude
SRC = src
OBJ = obj
BIN = bin

# Ficheiros
CLIENT_OBJS = $(SRC)/dclient.c $(SRC)/client.c $(SRC)/utils.c
SERVER_OBJS = $(SRC)/dserver.c $(SRC)/server.c $(SRC)/utils.c

# Targets
all: dclient dserver

dclient: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o dclient $(CLIENT_OBJS)

dserver: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o dserver $(SERVER_OBJS)

clean:
	rm -f dclient dserver
	rm -f /tmp/dclient_fifo /tmp/dserver_fifo

.PHONY: all clean