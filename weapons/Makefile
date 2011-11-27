CC = gcc
DEBUG = -g
WARNFLAGS = -Wall -Werror
SSLFLAGS = -lssl -lcrypto
INCLUDE = ../includes/smartalloc.c

all: weapons sweapons

weapons:
	$(CC) $(DEBUG) $(WARNFLAGS) -pthread -c -o weapons.o weapons.c -Wpacked $(SSLFLAGS)

sweapons: weapons
	$(CC) $(DEBUG) $(WARNFLAGS) $(INCLUDE) -pthread -o sweapons weapons.o $(SSLFLAGS)

clean:
	rm -rf *~ 
