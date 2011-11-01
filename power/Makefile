C = gcc
CFLAGS = -g -Wall -Werror

all:  power ship

power: 
	$(CC) -Wall -Werror -pthread -c -o pwr.o power.c -Wpacked 

ship: pwr.o
	$(CC) $(CFLAGS) -pthread -o spwr spower.c ../includes/smartalloc.c pwr.o

clean:
	rm pwr.o spwr
