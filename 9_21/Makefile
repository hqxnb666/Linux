# Makefile for building the multi-threaded server

CC=gcc
CFLAGS=-Wall -g
LIBS=-lpthread

# Target executable name
TARGET=server_mt

all: $(TARGET)

$(TARGET): server_mt.c
	$(CC) $(CFLAGS) -o $(TARGET) server_mt.c $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

