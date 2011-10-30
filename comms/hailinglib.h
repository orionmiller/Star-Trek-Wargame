/**
@author Dirk Cummings
@author Orion Miller
@version White Hat - Star Trek CTF
@version Fall 2011

*/

void comms_setup(void);
void comms_startup(void);
void comms_shutdown(void);

int getPwrAlloc(void); //pretty much copy & past from eng.c
void *request_handler(void *in);
void send_packet(struct EngineHeader *, int);
void req_report(int confd);
void print_comms_status(void);

void send_msg(void *msg);
void recv_msg(void *msg);

/* start up */

/* aquire power from the power service */

/* pkt - hdr */
/* team #:challenge:#: */

/* craft packet  */

/* check state/ test */
/* check to see how much power i have */
/* if it is below a certain amount - go offline */

/* user modified
   encryption & decryption
   
*/
