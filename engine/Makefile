CC = gcc
CFLAGS = -g -Wall -Werror

all:  eng ship

eng:
	$(CC) -pthread -c -o eng.o eng.c -Wpacked

ship: eng.o
	$(CC) $(CFLAGS) -pthread -o seng seng.c ../includes/smartalloc.c eng.o

clean:
	rm -rf eng.o seng
