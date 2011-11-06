CC = gcc
DEBUG = -g
WARNFLAGS = -Wall -Werror
SSLFLAGS = -lssl -lcrypto

all:  power ship

power: 
	$(CC) $(WARNFLAGS) -pthread -c -o pwr.o power.c -Wpacked 

ship: pwr.o
	$(CC) $(WARNFLAGS) -pthread -c -o spwr.o spower.c 
	$(CC) $(DEBUG) $(WARNFLAGS) -pthread -o spwr spwr.o pwr.o $(SSLFLAGS)

clean:
	rm pwr.o spwr.o spwr
