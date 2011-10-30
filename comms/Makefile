
CC = gcc
CFLAGS = -g -Wall -Werror

#Dependencies


#Sources
COMMS_SRCS = comms.c
SCOMMS_SRCS = scomms.c comms.o

all: comms scomms

hailinglib:
	echo "Do Nothing"
comms:
	$(CC) $(CFLAGS) -c $(COMMS_SRCS) -pthread -o comms.o

scomms: comms
	$(CC) $(CFLAGS) $(SCOMMS_SRCS) -pthread -o scomms

hq:
	echo "Do Nothing"

clean:
	@rm -f *~ *.o

allclean:
	echo "Do Nothing"