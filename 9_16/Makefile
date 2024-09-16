CC = gcc
CFLAGS = -Wall -O2

all: clock server

clock: clock.o timelib.o
	$(CC) $(CFLAGS) -o clock clock.o timelib.o

server: server.o timelib.o
	$(CC) $(CFLAGS) -o server server.o timelib.o

clock.o: clock.c timelib.h
	$(CC) $(CFLAGS) -c clock.c

server.o: server.c timelib.h
	$(CC) $(CFLAGS) -c server.c

timelib.o: timelib.c timelib.h
	$(CC) $(CFLAGS) -c timelib.c

clean:
	rm -f *.o clock server

