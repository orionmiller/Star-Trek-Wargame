
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
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
//#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../includes/smartalloc.h"
#include "../includes/pwr_packet.h"
#include "../includes/weapons_packet.h"

#ifndef WEAPONS_H_
#define WEAPONS_H_

#define LOG_FILE_LOCATION "../logs/weapons.log"
#define LOG_FILE_PERMS 0666
#define IP_ADDRESS "127.0.0.1"

/* The file desciptor for writing to the Log file */
int logfd;

/* 
   This structure contains functions pointers to all necessary functions to
      implement the power system. 
   
   There is a single instance of this structure in the power service. The 
      fields can be accessed or changed in any portion of your code and should
      be considered as a global variable.
   
   Do not delcare another instance of this structure.
*/ 
extern struct  WeaponsFuncs {

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
  
  /*
    Send a status report Engine Packet through the confd socket.
  */
  void (*request_report) (int confd);
  
  /*
    If you do not wish to have write a special utility to build and send 
    packages to your sevices, you may write your own command line interface
    to control this service. 
    
    You may, for example, implement an RPC like interface or an interactive 
    interface by listening on a port. The implementation is up to you; just
    remember to make it secure.
    
    This functionality is NOT supported out of the box. That is, there is no
    provided interm implemenation. This is purely for your convenience and
    thus you must provide it yourself.
    
    If this function IS set, then a new pthread will be created and your function
    called with NULL passed as the input parameter. 
  */
  void *(*cmd_line_inter) (void *);
  
  /*
    The following functions are your wrappers around the built-in / main Power
    service functions. They will be used for testing your service 
    implementation and will be called until your write your own request
    handler. At the end of each of your implementations, you must call the 
    external equivalent functions. A diagram of the setup is bellow along 
    with an example of the order of the function calls.
    
    Diagram:
    
    ----------------------------------------------------------
    |  request_handler                                       |
    |                                                        |
    |     ---------------------------------------------------|
    |     |  additional_function                             |
    |     |     (if you want to add additional               |
    |     |     checking or functionality to add security)   |
    |     |                                                  |
    |     |     ---------------------------------------------|
    |     |     |  wrapper_function                          |
    |     |     |     (for your book-keeping)                |
    |     |     |     EX) eng_funcs.wwarp_speed(int)         |
    |     |     |     ---------------------------------------|
    |     |     |     |  built-in_function                   |
    |     |     |     |     EX) add_power(int, int)          |
    |     |     |     |                                      |
    ----------------------------------------------------------
    
    
    
    Function Call Example:
    
    --> Incoming Connection Request
        |--> Connection Accepted
           |--> eng_funcs.request_handler(connection_file_descriptor)
               |--> eng_funcs.wengage_impulse(int)
               |--> engage_impulse(int)
    
    You MUST send your data packet AFTER you call the built-in functions.
    Your implementation correctness can not be guaranteed otherwise.
    
    Your wrapper functions should return -1 on ANY error which results
    in the request not being processed or completed.
  */


  int (*fire_weapons) (int,int);


}weapons_funcs;

/*
  Main entry point into this service. Sets up all book/house-keeping and
  starts service's logic.
  
  This function can not be replaced and must be called to start your service
  implementation.
*/
extern void weapons_startup(void);

/*
  Main exit to this service. Must be called to perform house-keeping cleanup
  and shutdown all other registered services.
  
  This function can not be replaced and must be called before your service
  exits.
*/
extern void weapons_shutdown(void);

/*
  Startup the impulse drive with initial speed. Possible speeds are indexed
  starting at Zero and are one of the following:
  -   0 ,  1 ,  2 ,  3 ,  4 ,  5 ,  6
  - Stop, 1/4, 1/3, 1/2, 2/3, 3/4, Full
  
  RETURNS the current Impulse Speed Index, or -1 on error.
*/
extern int engage_impulse(int speed);

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
  
  RETURNS the current Impulse Speed Index, or -1 on error.
*/
extern int impulse_speed(int speed);


/*
  Sends data out to the previously connect(2)'ed TCP connection using 
  'socket_fd'. You should set-up your sockaddr_t, use socket(2) and
  then connect(2) before you call this function to send data out. 
  
  You must use this function to send packets to other services. If you
  do not use this function, you may fail the basic functionality
  tests and lose points.
  
  RETURNS 1 when the send was successful, -1 on error, or 0 when testing.
  When testing and 0 is returned, you should ignore all previous
  changes made during the wrapper function call. ie, don't save the
  state.
*/
extern int send_packet(struct WeaponsHeader *pck, int socket_fd);


#endif
