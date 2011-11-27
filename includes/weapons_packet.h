
#ifndef WEAPONS_PKT_H_
#define WEAPONS_PKT_H_

#define STATIC_WEAPONS_PORT 3843

/* Services to Shoot At */
#define 

#define NUM_SRVCS = 6
#define ENGINE_SRVC =0
#define NAVIGATION_SRVC=1
#define POWER_SRVC =2
#define WEAPONS_SRVC =3
#define SHIELDS_SRVC =4
#define COMMS_SRVC =5   

/*
   NOTES:
      - The Main Weapons service uses and catches SIGINT and SIGALRM. Do not
         use or block these signals or your implementation will not pass
         any of the tests and your team will lose points.
      - The service does nothing with any other signal. Feel free to use
         then if your implementation requires.
*/


/*
The Structure of the Weapons Packet is as follows:

 0                         7                        15
 |---------------------------------------------------|
 |        Version          |     Length of Packet    |
 |---------------------------------------------------|
 16                       23                        31
 |---------------------------------------------------|
 |         Service         |    Number of Shots      |
 |---------------------------------------------------|

Service:
  Is which service you would like to fire at.

Number of Shots:
  How many times you want to fire upon that service.
  
Notes:
  - There is a queue of shots that will be 


*/

typedef struct {
  uint8_t ver;
  uint8_t len;
  uint8_t service;
  uint8_t num_shots;
}WeaponsHeader;

#endif
