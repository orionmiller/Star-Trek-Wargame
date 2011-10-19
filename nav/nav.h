#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../includes/smartalloc.h"
#include "../includes/pwr_packet.h"
#include "../includes/eng_packet.h"
#include "../includes/nav_packet.h"

/* See "man listen(2)" for second parameter */
#define BACKLOG 15
/* Max Buffer for receiving data from connection */
#define MAXBUF 4096
/* Log file Location */
#define LOG_FILE_LOCATION "../logs/navigation.log"
#define LOG_FILE_PERMS 0666

/* 
   This structure contains functions pointers to all necessary functions to
      implement the power system. 
   
   There is a single instance of this structure in the power service. The 
      fields can be accessed or changed in any portion of your code and should
      be considered as a global variable.
   
   Do not delcare another instance of this structure.
*/ 
extern struct NavigationFuncs { 
   /*
      For receiving and processing data from a socket. This function will be 
         called in a new thread once a successful connection has been made 
         across a socket.

      The void * parameter should be a valid pointer to a file descriptor 
         returned from accept(2).

      Since this function definition is for a function pointer, you can replace
         the built in function with your own.
         
      Replace the built-in function with your own with this line:
         nav_funcs.request_handler = &YourFunctionName;
      
      Your function should be declared like:
         void *YourFunctionName(void *in)
   */
   void *(*request_handler) (void *confd);
} nav_funcs;

/*
   Main entry point into this service. Sets up all book/house-keeping and
      starts service's logic.

   This function can not be replaced and must be called to start your service
      implementation.
*/
extern void navigation_startup(void);

/*
   Main exit to this service. Must be called to perform house-keeping cleanup
      and shutdown all other registered services.
   
   This function can not be replaced and must be called before your service
      exits.
*/
extern void navigation_shutdown(void);