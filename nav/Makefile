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

all:  nav-$(EXEC_SUFFIX) snav-$(EXEC_SUFFIX)

nav-$(EXEC_SUFFIX): nav.c
	$(CC) $(CFLAGS) $(OSINC) $(OSLIB) $(OSDEF) -pthread -c -o nav-$(EXEC_SUFFIX).o nav.c -Wpacked

snav-$(EXEC_SUFFIX): snav.c
	$(CC) $(CFLAGS) $(OSINC) $(OSLIB) $(OSDEF) -pthread -o snav-$(EXEC_SUFFIX).o snav.c ../includes/smartalloc.c nav-$(EXEC_SUFFIX).o

clean:
	rm -rf nav-* snav-*
