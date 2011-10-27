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

int addPower(int, int);
int transPower(int, int, int);
int freePower(int, int);
int reg_service(int, int);
int unreg_service(int);
void *reqHandler(void *);

int main(int argc, char **argv)
{
   // Setup the Function Pointers
//   pow_funcs.request_handler = &reqHandler;
   pow_funcs.wadd_power = &addPower;
   pow_funcs.wregister_service = &reg_service;
   pow_funcs.wunregister_service = &unreg_service;
   pow_funcs.wtransfer_power = &transPower;
   pow_funcs.wfree_power = &freePower;


   printf("Starting Up\n");
   power_startup();

   printf("Shutting Down\n");
   power_shutdown();
   printf("Shut Down\n");
   
   return 0;
}

int addPower(int amt, int dest)
{
   // Call built-in add_power() at end of function
   return add_power(amt, dest);
}

int transPower(int amt, int src, int dest)
{
   return transfer_power(amt, src, dest);
}

int freePower(int amt, int dest)
{
   // Call built-in free_power() at end of function
   return free_power(amt, dest);
}

int reg_service(int svcId, int pid)
{
   return register_service(svcId, pid);
}

int unreg_service(int svcId)
{
   return unregister_service(svcId);
}

void *reqHandler(void *in)
{
   // Your implementation

   return (NULL);
}
