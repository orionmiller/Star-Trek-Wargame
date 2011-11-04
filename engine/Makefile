CC = gcc
DEBUG = -g
CFLAGS = -Wall -Werror -lssl -lcrypto

all:  eng ship

eng:
	$(CC) $(CFLAGS) -pthread -c -o eng.o eng.c -Wpacked

ship: eng.o
	$(CC) $(DEBUG) $(CFLAGS) -pthread -o seng seng.c ../includes/smartalloc.c eng.o

clean:
	rm -rf eng.o seng
