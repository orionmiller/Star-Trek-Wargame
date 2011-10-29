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

int setCourse(int, int, int);
void *reqHandler(void *);

int main(int argc, char **argv)
{
   // Setup the Function Pointers
//   nav_funcs.request_handler = &reqHandler;
   nav_funcs.wset_course = &setCourse;

   printf("Starting Up\n");
   navigation_startup();
   printf("Started\n");

   printf("Shutting Down\n");
   navigation_shutdown();
   printf("Shut Down\n");
   
   return 0;
}

int setCourse(int crs_dir, int eng_type, int speed)
{
   return set_course(crs_dir, eng_type, speed);
}

void *reqHandler(void *in)
{
   // Your implementation
   
   return (NULL);
}
