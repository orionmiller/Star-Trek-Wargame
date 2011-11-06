CC = gcc
DEBUG = -g
WARNFLAGS = -Wall -Werror
SSLFLAGS = -lssl -lcrypto

all:  nav ship

nav:
	$(CC) $(WARNFLAGS) -pthread -c -o nav.o nav.c -Wpacked

ship:
	$(CC) $(WARNFLAGS) -pthread -c -o snav.o snav.c 
	$(CC) $(WARNFLAGS) $(DEBUG) -pthread -o snav snav.o nav.o $(SSLFLAGS)

clean:
	rm -rf nav.o snav
