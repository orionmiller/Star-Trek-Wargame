/**
Main "Comms" service. To be compiled & obfuscated before given to contestants.

Contestants must use and implement "eng.c"

No main function included (the contestants will have this).
   Replaced with "engine_startup()". Therefo, must compile with
   "gcc -c" option.

Compile as: cc -c eng.c -o commsd.o -lpthread -Wpacked
  or use the Makefile

@author Orion Miller
@editor Dirk Cummings
@version White Hat Star Trek CTF
@version Fall 2011
 */

#include "comms.h"


typedef struct {
  uint32_t pwr_alloc;
}status_struct;

status_struct cstat = {0};
comms_funcs comm_funcs = {&request_handler, &encrypt, &decrypt};


void *request_handler(void *in)
{
  printf("Requesting");
  //get challenge #
  //send challenge # response
  return NULL;
}

int encrypt(uint8_t *data)
{
  //--Do Nothing--//
  return 1; //magic number
}


int decrypt(uint8_t *data)
{
  //--Do Nothing--//
  return 1; //magic number
}


void *cli_interface(void)
{
  return NULL;
}





void get_msg_from_hq(void);
void send_msg_to_hq(void);

void comms_startup()
{
  printf("Comms Sarting Up\n");
  //  printf("power: %d", getPwrAlloc());
}


void comms_shutdown()
{

}

void get_problem(int challenge_num, uint8_t *data)
{
  
}

void send_solution(int challenge_num, uint8_t *data)
{

}

int getPwrAlloc(void)
{
   struct sockaddr_in pwr_sck;
   /* Power header */
   struct PowerHeader *pwrhd;
   char tmp[1];
   int pwr_sockfd;

   /* Connect to Power Service and register self */
   bzero(&pwr_sck, sizeof(pwr_sck));
   
   if(inet_pton(AF_INET, IP_ADDRESS, &(pwr_sck.sin_addr)) > 0)
   {
      pwr_sck.sin_port = (STATIC_PWR_PORT);
      pwr_sck.sin_family = AF_INET;
   }
   else
   {
      perror("Bad IP Address");
      return -1;
   }
   
   if((pwr_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket Error");
      return -1;
   }

   if(connect(pwr_sockfd, (struct sockaddr *)&pwr_sck, sizeof(pwr_sck)) == -1)
   {
      perror("Power Connecting Error");
      close(pwr_sockfd);
      return -1;
   }

   pwrhd = (struct PowerHeader *)malloc(sizeof(struct PowerHeader));
   pwrhd->ver = 1;
   pwrhd->len = sizeof(struct PowerHeader);
   pwrhd->src_svc = COMMS_SVC_NUM;
   pwrhd->dest_svc = POWER_SVC_NUM;
   pwrhd->req_type_amt = (REG_NEW_SVC << 4);
   pwrhd->pid = getpid();

   /* Get current power allocation */
   if(write(pwr_sockfd, (void *)pwrhd, sizeof(struct PowerHeader)) < 0)
   {
      perror("Communicating to Power Service");
      free(pwrhd);
      return -1;
   }

   tmp[0] = '\0';
   if(write(pwr_sockfd, (void *)tmp, strlen(tmp) + 1) < 0)
   {
      perror("Terminating registration");
      free(pwrhd);
      return -1;
   }

   if(read(pwr_sockfd, (void *)pwrhd, sizeof(struct PowerHeader)) < 0)
   {
      perror("Receiving from Power Service");
      free(pwrhd);
      return -1;
   }

   cstat.pwr_alloc = pwrhd->alloc;

   free(pwrhd);

   return cstat.pwr_alloc;
}
