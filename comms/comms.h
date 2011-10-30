

#ifndef COMMS_H_
#define COMMS_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "../includes/pwr_packet.h"

#define BACKLOG 15
#define LOG_FILE_LOCATION "../logs/comms.log"
#define IP_ADDRESS "127.0.0.1"



extern struct comms_funcs_t{
  void *(*req_handler) (void *confd);
  int (*encrypt) (uint8_t *data);
  int (*decrypt) (uint8_t *data);
}comm_funcs;

typedef struct comms_funcs_t comms_funcs;

/*
                   COMMS HDR

 0---------------------7---------------------15
 |       Opcode        |   Challenge Number   | 
 ----------------------------------------------

 Opcode
   Specifying if you want to send a solution to an encrypted packet
   or to receive a problem for HQ.

 Challenge NUmber
   Which challenge you would like to send HQ or receive from HQ.

 */

extern struct comms_hdr_t {
  uint8_t opcode;
  uint8_t challange_num;
}comms_hdr;

//   Prototypes   //
extern void *request_handler(void *in);
extern int encrypt(uint8_t *data);
extern int decrypt(uint8_t *data);

#endif
