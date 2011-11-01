CC = gcc
CFLAGS = -g -Wall -Werror

all:  nav ship

nav:
	$(CC) -pthread -c -o nav.o nav.c -Wpacked

ship: nav.o
	$(CC) $(CFLAGS) -pthread -o snav snav.c ../includes/smartalloc.c nav.o

clean:
	rm -rf nav.o snav
