CC = gcc
DEBUG = -g
WARNFLAGS = -Wall -Werror
SSLFLAGS = -lssl -lcrypto
INCLUDE = ../includes/smartalloc.c
CFLAGS = $(DEBUG) $(WARNFLAGS) -Wpacked

LIBLOG_DEP = liblog.c liblog.h
LIBLOG_SRC = liblog.c

LIBSERVICE_DEP = libservice.c liblog libconnect
LIBSERVICE_SRC = libservice.c liblog.o libconnect.c

LIBCONNECT_DEP = libconnect.h libconnect.c liblog
LIBCONNECT_SRC = libconnect.c liblog.o

all: liblog libconnect libservice

libservice: $(LIBSERVICE_DEP)
	$(CC) $(CFLAGS) -c $(LIBSERVICE_SRC) -o libservice.o 

libconnect: $(LIBCONNECT_DEP)
	$(CC) $(CFLAGS) $(SSLFLAGS) -c libconnect.c -o libconnect.o 

liblog: $(LIBLOG_DEP)
	$(CC) $(CFLAGS) -c $(LIBLOG_SRC) -o liblog.o 

clean:
	@rm -f *~

allclean: clean
	@rm -f libservice.o liblog.o libconnect.o