CC = gcc
DEBUG = -g
WARNFLAGS = -Wall -Werror
SSLFLGAS = -lssl -lcrypto
INCLUDE = ../includes/smartalloc.c

all:  eng ship

eng:
	$(CC) $(WARNFLAGS) -pthread -c -o eng.o eng.c -Wpacked

ship:
	$(CC) $(WARNFLAGS) -pthread -c -o seng.o seng.c  
	$(CC) $(DEBUG) $(WARNFLAGS) $(INCLUDE) -pthread -o seng seng.o eng.o $(SSLFLAGS)

clean:
	rm -rf eng.o seng.o seng
