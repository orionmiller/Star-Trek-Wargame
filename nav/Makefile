CC = gcc
DEBUG = -g
WARNFLAGS = -Wall -Werror
SSLFLAGS = -lssl -lcrypto
INCLUDE = ../includes/smartalloc.c

all:  nav ship

nav:
	$(CC) $(WARNFLAGS) -pthread -c -o nav.o nav.c -Wpacked

ship:
	$(CC) $(WARNFLAGS) -pthread -c -o snav.o snav.c 
	$(CC) $(WARNFLAGS) $(DEBUG) -pthread -o snav snav.o nav.o $(INCLUDE) $(SSLFLAGS)

clean:
	rm -rf nav.o snav
