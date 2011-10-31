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

/* See "man listen(2)" for second parameter */
#define BACKLOG 15
/* Max Buffer for receiving data from connection */
#define MAXBUF 4096
/* Log file Location */
#define LOG_FILE_LOCATION "../logs/power.log"
#define LOG_FILE_PERMS 0666
#define IP_ADDRESS "127.0.0.1"

/* The file descriptor for writing to the Log file */
int logfd;

/*
   NOTES:
      - The Main Power service uses and catches SIGINT and SIGALRM. Do not
         use or block these signals or your implementation will not pass
         any of the tests and your team will lose points.
      - The service does nothing with any other signal. Feel free to use
         then if your implementation requires.
      - For the definition of a Power Packet, see "pwr_packet.h"
*/

/* 
   This structure contains functions pointers to all necessary functions to
      implement the power system. 
   
   There is a single instance of this structure in the power service. The 
      implement the power system. 
   
   There is a single instance of this structure in the power service. The 
      fields can be accessed or changed in any portion of your code and should
      be considered as a global variable.
   
   Do not delcare another instance of this structure.
*/ 
extern struct PowerFuncs {     
   /*
      For receiving and processing data from a socket. This function will be 
         called in a new thread once a successful connection has been made 
         across a socket.

      The void * parameter should be a valid pointer to a file descriptor 
         returned from accept(2).

      Since this function definition is for a function pointer, you can replace
         the built in function with your own.
         
      Replace the built-in function with your own with this line:
         pow_funcs.request_handler = &YourFunctionName;
      
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
      |     |     |     EX) pow_funcs.wadd_power(int, int)     |
      |     |     |     ---------------------------------------|
      |     |     |     |  built-in_function                   |
      |     |     |     |     EX) add_power(int, int)          |
      |     |     |     |                                      |
      ----------------------------------------------------------



      Function Call Example:
         
      --> Incoming Connection Request
            |--> Connection Accepted
            |--> pow_funcs.request_handler(connection_file_descriptor)
               |--> pow_funcs.wadd_power(int, int)
                  |--> add_power(int, int)
   */

   int (*wregister_service) (int, int);

   int (*wunregister_service) (int);

   int (*wadd_power) (int, int);
    
   int (*wtransfer_power) (int, int, int);

   int (*wfree_power) (int, int);
} pow_funcs;

/*
   Main entry point into this service. Sets up all book/house-keeping and
      starts service's logic.

   This function can not be replaced and must be called to start your service
      implementation.
*/
extern void power_startup(void);

/*
   Main exit to this service. Must be called to perform house-keeping cleanup
      and shutdown all other registered services.
   
   This function can not be replaced and must be called before your service
      exits.
*/
extern void power_shutdown(void);

/*
   This function must be called in order to register a new service with this
      power service. When a new service comes online, call this function with
      the new service's Id and its process Id.
      
   The Source and Destination Service for a Registration Request can be used
      interchangeably. 

   Returns the svc_id of the newly registered service if successful, otherwise
      returns 0.
*/
extern int register_service(int svc_id, int svc_pid);

/*
   This function must be called when a service needs to be unregistered because
      it is going off-line or has become unresponsive. Any registered service 
      which is off-line or unresponsive and has not been unregistered will 
      still be allocated power which may prevent efficient power allocated/use
      for other services.
      
   The Source and Destination Service for a Un-Registration Request can be used
      interchangeably.

   Returns the now unregistered service's id on success, otherwise 0.
*/
extern int unregister_service(int svc_id);

/*
   Transfers Amount of Allocation power from the Source service to the 
      Destination Service.
   
   Checks:
      - not allocating more than the Maximum amount of Power available
         for allocation.
      - Source and Destination are valid Ids and services have been properly
         registered.
      
   This is a built-in function which can not be replaced and must be used to 
      ensure proper book-keeping.
      
   NOTE:
      In the event the transfering of power results in the depletion of power
         for the Source Service, the signal SIGTSTP will be sent to the Source
         Service's Process until such time as power can be restored.
   
   Returns the total amount of newly allocated power to the Destination
      Service, or -1 on error.
*/
extern int transfer_power(int amt, int src, int dest);

/*
   Add Amount of power to Destination's Allocation if there is enough Reserve 
      Power remaining. If the Reserve Power remaining is less than Amount, 
      only the difference is added to Destination.
      
   The Source Service of the Allocation Request is the same as the Destination
      - not allocating more than the Maximum amount of Ship Power available 
      - Destination is a valid and properly registered service.

   This is a built-in function which can not be replaced and must be used to 
      ensure proper book-keeping.
      
   NOTE:
      In the event the Destination Service's allocation was previously 0,
         and thus was sent a SIGTSTOP, the Destination Service Process is
         sent a SIGCONT to break the SIGTSTOP.
      
   Returns the total amount of newly allocated power to the Destination
      Service, or -1 on error.
*/
extern int add_power(int amt, int dest);

/*
   Removes Amount of power from Destination's Allocation.
   
   The Source Service of the De-Allocation Request is the same as the
      Destionation.

   Checks: 
      - Destination is a vaild and properly registered service.

   This is a built-in function which can not be replaced and must be used to
      ensure proper book-keeping.
      
   NOTE:
      In the event the Destination Service's de-allocation of Power results
         in 0 units of power allocated to Destination, then a signal of 
         SIGTSTP is sent to the Destination's Process.
      
   Returns the total amount of power actually de-allocated from the Destination
      Service, or -1 on error.
*/
extern int free_power(int amt, int dest);

/*
   Prints the current list of registered services and their status for 
      debugging and visual reporting.
*/
extern void print_power_status();
