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

all:  eng-$(EXEC_SUFFIX) seng-$(EXEC_SUFFIX)

eng-$(EXEC_SUFFIX): eng.c
	$(CC) $(CFLAGS) $(OSINC) $(OSLIB) $(OSDEF) -pthread -c -o eng-$(EXEC_SUFFIX).o eng.c -Wpacked

seng-$(EXEC_SUFFIX): seng.c
	$(CC) $(CFLAGS) $(OSINC) $(OSLIB) $(OSDEF) -pthread -o seng-$(EXEC_SUFFIX).o seng.c ../includes/smartalloc.c eng-$(EXEC_SUFFIX).o

clean:
	rm -rf eng-* seng-*
