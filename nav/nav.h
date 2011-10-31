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
#define BACKLOG 1
/* Max Buffer for receiving data from connection */
#define MAXBUF 4096
/* Log file Location */
#define LOG_FILE_LOCATION "../logs/navigation.log"
#define LOG_FILE_PERMS 0666
#define IP_ADDRESS "127.0.0.1"

#define PASABLE_OBJ 1
#define IMPASSABLE_OBJ 2
#define ENEMY_SHIP 3

/* The file descriptor for writing to the Log file */
int logfd;

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
      |     |     |     EX) nav_funcs.wset_course(int,int,int) |
      |     |     |     ---------------------------------------|
      |     |     |     |  built-in_function                   |
      |     |     |     |     EX) set_course(int, int, int)    |
      |     |     |     |                                      |
      ----------------------------------------------------------



      Function Call Example:
         
      --> Incoming Connection Request
            |--> Connection Accepted
            |--> nav_funcs.request_handler(connection_file_descriptor)
               |--> nav_funcs.wset_course(int, int, int)
                  |--> set_course(int, int, int)
   */
   int (*wset_course) (int, int, int);
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

/*
   
   
   This function can not be replaced and must be called at the end of your 
      implementation.
      
   crs_dir - Verified Course Direction. Must be one of the definitions listed
      in the `nav_packet.h` prefixed with "CRS_".
   
   eng_type - The type of engine to set the `speed` of. Must be one of the 
      definitions listed in the `eng_packet.h` file.
   
   speed - The speed index for the `eng_type` to set. Use the indexes described
      `eng_packet.h`
      
   THINGS TO NOTE:
      - You can NOT change directions while the warp engines are engaged.
      - You can ONLY change speed while the warp engines are engaged.
      - You can change directions AND speed while impulse engines are engaged.
      - This function IS thread safe.
   
   RETURNS 1 on success, 0 when only one condition could be met, 
      or -1 otherwise.
*/
extern int set_course(int crs_dir, int eng_type, int speed);
