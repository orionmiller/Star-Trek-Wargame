CC = gcc
DEBUG = -g
CFLAGS = -Wall -Werror -lssl -lcrypto

all:  power ship

power: 
	$(CC) $(CFLAGS) -Wall -Werror -pthread -c -o pwr.o power.c -Wpacked 

ship: pwr.o
	$(CC) $(DEBUG) $(CFLAGS) -pthread -o spwr spower.c ../includes/smartalloc.c pwr.o

clean:
	rm pwr.o spwr
