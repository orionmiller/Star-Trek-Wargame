/**
Your ship's version of a navigation system. Include and implement the navigation's
functions and security features.

Must pass base functionality to be considered "up"

Use the Makefile to compile or use:    gcc nav.o snav.c -o snav.o -lpthread

@author Dirk Cummings
@author Orion Miller
@version White Hat Star Trek CTF
@version Fall 2011
*/

#include "nav.h"

void engageImpulse(void);
void impulseSpeed(void);
void engageWarp(void);
void warpSpeed(void);
void reqHandler(void *);

int main(int argc, char **argv)
{
   // Setup the Function Pointers
//   nav_funcs.request_handler = &reqHandler;

   printf("Starting Up\n");
   navigation_startup();
   printf("Started\n");

   printf("Shutting Down\n");
   navigation_shutdown();
   printf("Shut Down\n");
   
   return 0;
}

void engageImpulse(void)
{

}

void impulseSpeed(void)
{

}

void engageWarp(void)
{

}

void warpSpeed(void)
{

}

void reqHandler(void *in)
{
   // Your implementation
}
