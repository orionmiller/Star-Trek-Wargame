/**
Main "Engine" service. To be compiled before given to contestants.

Contestants must use and implement "eng.c"

No main function included (the contestants will have this). 
   Replaced with "engine_startup()". Therefor, must compile with 
   "gcc -c" option.

Compile as:    gcc -c eng.c -o engd.o -lpthread -Wpacked
  or use the Makefile

@author Dirk Cummings
@author Orion Miller
@version White Hat Star Trek CTF
@version Fall 2011

*/

#include "weapons.h"
#include <time.h>


/* 
   DECRYPTING DEFINES ORDER
   XOR -> SWAP -> SUB -> FLIP
*/
#define SWAP(pass) ((*pass)=(((*pass)&0x0F)<<4) | (((*pass)&0xF0)>>4))
#define XOR(pass) ((*pass)=(*pass)^0x7B)
#define SUB(pass) ((*pass)-=7)
#define FLIP(pass) ((*pass)=~(*pass))



