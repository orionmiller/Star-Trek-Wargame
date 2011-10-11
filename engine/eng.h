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
#include "smartalloc.h"
#include "pwr_packet.h"

/* The static port for the Main Engine Service to run on */
#define STATIC_ENG_PORT 3723
/* See "man listen(2)" for second parameter */
#define BACKLOG 15
/* Max Buffer for receiving data from connection */
#define MAXBUF 4096
/* Log file Location */
#define LOG_FILE_LOCATION "./logs/engine.log"
#define LOG_FILE_PERMS 0666

/* 
   This structure contains functions pointers to all necessary functions to
      implement the power system. 
   
   There is a single instance of this structure in the power service. The 
      fields can be accessed or changed in any portion of your code and should
      be considered as a global variable.
   
   Do not delcare another instance of this structure.
*/ 
extern struct EngineFuncs { 
   /*
      For receiving and processing data from a socket. This function will be 
         called in a new thread once a successful connection has been made 
         across a socket.

      The void * parameter should be a valid pointer to a file descriptor 
         returned from accept(2).

      Since this function definition is for a function pointer, you can replace
         the built in function with your own.
         
      Replace the built-in function with your own with this line:
         eng_funcs.request_handler = &YourFunctionName;
      
      Your function should be declared like:
         void *YourFunctionName(void *in)
   */
   void *(*request_handler) (void *confd);
} eng_funcs;

/*
   Main entry point into this service. Sets up all book/house-keeping and
      starts service's logic.

   This function can not be replaced and must be called to start your service
      implementation.
*/
extern void engine_startup(void);

/*
   Main exit to this service. Must be called to perform house-keeping cleanup
      and shutdown all other registered services.
   
   This function can not be replaced and must be called before your service
      exits.
*/
extern void engine_shutdown(void);

/*
   Startup the impulse drive with initial speed. Possible speeds are indexed
      starting at Zero and are one of the following:
      -   0 ,  1 ,  2 ,  3 ,  4 ,  5 ,  6
      - Stop, 1/4, 1/3, 1/2, 2/3, 3/4, Full
      
*/
extern void engage_impulse(int speed);

/*
   Incrase or Decrease inpulse speed. Possible speeds are:
      - Stop, 1/4, 1/3, 1/2, 2/3, 3/4, Full
   
   Where Full is 1/4th the speed of light (or approx. 74,948,114 m/s or 
      269,813,210 kph).
      
   To Increase or Decrease the current speed, simple call with the index of
      the speed you wish to change to described in `engage_impulse`.
      
   NOTE:
      The impulse drives generate a variable amount of heat depending on the
         speed at which they are engaged. Overheating will cause damage to the 
         drives and can cost you points if they go out of service.
*/
extern void impulse_speed(int speed);

/* 
   Startup the warp drive with initial speed. Possible speeds are indexed
      starting at Zero and are one of the following:
      -   0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9
      - Stop, 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9
      
   NOTE:
      The warp drive generates considerably more heat than the impulse drives
         and thus can cause overheating quicker. 
*/
extern void engage_warp(int speed);

/*
   Increase or Decrease your ship's warp speed. See `engage_warp` for the list
      of possible speeds and their indexes.

   To change warp speed, simply call this function with the index of the 
      desired speed described in `engage_warp`.
*/
extern void warp_speed(void);
