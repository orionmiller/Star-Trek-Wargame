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

#include "weapons.h"

int fireWeapons(int, int);
void *reqHandler(void *);

int main(int argc, char **argv)
{
   // Setup the Function Pointers
//   eng_funcs.request_handler = &reqHandler;
   eng_funcs.wengage_impulse = &engageImpulse;
   weapons_funcs.fire_weapons = &fireWeapons;



   printf("Starting Up\n");
   weapons_startup();   
   printf("Shutting Down\n");
   weapons_shutdown();
   printf("Shut Down\n");

   return 0;
}



int fireWeapons(int service, int num_shots)
{
  return fire_weapons(service, num_shots);
}


void *reqHandler(void *in)
{
   // Your implementation
   return (NULL);
}
