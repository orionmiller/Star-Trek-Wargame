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

int engageImpulse(int);
int impulseSpeed(int);
int engageWarp(int);
int warpSpeed(int);
void *reqHandler(void *);

int main(int argc, char **argv)
{
   // Setup the Function Pointers
//   eng_funcs.request_handler = &reqHandler;
   eng_funcs.wengage_impulse = &engageImpulse;
   eng_funcs.wimpulse_speed = &impulseSpeed;
   eng_funcs.wengage_warp = &engageWarp;
   eng_funcs.wwarp_speed = warpSpeed;

   printf("Starting Up\n");
   engine_startup();   
   
   printf("Shutting Down\n");
   engine_shutdown();
   printf("Shut Down\n");

   return 0;
}

int engageImpulse(int speed)
{
   return engage_impulse(speed);
}

int impulseSpeed(int speed)
{
   return impulse_speed(speed);
}

int engageWarp(int speed)
{
   return engage_warp(speed);
}

int warpSpeed(int speed)
{
   return warp_speed(speed);
}

void *reqHandler(void *in)
{
   // Your implementation
   return (NULL);
}
