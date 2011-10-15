/**
Your ship's version of a engine system. Include and implement the engine's
functions and security features.

Must pass base functionality to be considered "up"

Use the Makefile to compile or use:    gcc eng.o seng.c -o seng.o -lpthread

@author Dirk Cummings
@author Orion Miller
@version White Hat Star Trek CTF
@version Fall 2011
*/

#include "eng.h"

void engageImpulse(void);
void impulseSpeed(void);
void engageWarp(void);
void warpSpeed(void);
void reqHandler(void *);

int main(int argc, char **argv)
{
   // Setup the Function Pointers
//   eng_funcs.request_handler = &reqHandler;

   printf("Starting Up\n");
   engine_startup();   
   
   printf("Shutting Down\n");
   engine_shutdown();
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
