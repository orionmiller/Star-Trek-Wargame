CC = gcc
CFLAGS = -g -Wall -Werror
OS = $(shell uname -s)
PROC = $(shell uname -p)
EXEC_SUFFIX=$(OS)-$(PROC)

ifeq ("$(OS)", "SunOS")
	OSLIB=-L/opt/csw/lib -R/opt/csw/lib -lsocket -lnsl
	OSINC=-I/opt/csw/include
	OSDEF=-DSOLARIS
else
ifeq ("$(OS)", "Darwin")
	OSLIB=
	OSINC=
	OSDEF=-DDARWIN
else
	OSLIB=
	OSINC=
	OSDEF=-DLINUX
endif
endif

all:  power ship

power: power.c
	$(CC) -Wall -Werror $(OSINC) $(OSLIB) $(OSDEF) -pthread -c -o pwrd-$(EXEC_SUFFIX).o power.c -Wpacked

ship: spower.c
	$(CC) $(CFLAGS) $(OSINC) $(OSLIB) $(OSDEF) -pthread -o spwrd-$(EXEC_SUFFIX).o spower.c ../includes/smartalloc.c pwrd-$(EXEC_SUFFIX).o

clean:
	rm -rf pwrd-* spwrd-*
