CC = gcc
DEBUG = -g
CFLAGS = -Wall -Werror -lssl -lcrypto

all:  nav ship

nav:
	$(CC) $(CFLAGS) -pthread -c -o nav.o nav.c -Wpacked

ship: nav.o
	$(CC) $(CFLAGS) $(DEBUG) -pthread -o snav snav.c ../includes/smartalloc.c nav.o

clean:
	rm -rf nav.o snav
