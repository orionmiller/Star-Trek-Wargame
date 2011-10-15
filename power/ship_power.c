/**
Your ship's version of a power system. Include and implement the power functions.

Must pass base functionality to be considered "up"

Use the Makefile to compile or use:    gcc pwrd.o ship_power.c -o spwrd.o -lpthread

@author Dirk Cummings
@author Orion Miller
@version White Hat Star Trek CTF
@version Fall 2011
*/

#include "power.h"

void sendPower(int, int, int, int);
void removePower(void *, void *, void *);
void freePower(void *, void *);
void reqHandler(void *);

int main(int argc, char **argv)
{
   // Setup the Function Pointers
//   pow_funcs.request_handler = &reqHandler;


   printf("Starting Up\n");
   power_startup();

   printf("Shutting Down\n");
   power_shutdown();
   printf("Shut Down\n");
   
   return 0;
}

void sendPower(int amt, int src, int dest, int confd)
{
   // Call built-in send_power() at end of function
}

void removePower(void *amt, void *dest, void *confd)
{
   // Call built-in remove_power() at end of function
}

void freePower(void *amt, void *dest)
{
   // Call built-in free_power() at end of function
}

void reqHandler(void *in)
{
   // Your implementation
}
