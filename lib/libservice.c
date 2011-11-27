#include "libservice.h"

volatile sig_atomic_t running;
volatile sig_atomic_t testing;

extern void run_tests(void);

/*needs to emphasize a set up of connectoin not actual exchange of packets*/
conn *connect_pwr()
{
   int pwr_sockfd;
  struct sockaddr_in pwr_sck;
   /* Connect to Power Service and register self */
   bzero(&pwr_sck, sizeof(pwr_sck));
   pwr_sck.sin_family = AF_INET;

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
}

int create_pwr_hdr(pwr Hdr, int src_svc)
{
   Hdr->ver = 1;
   Hdr->len = sizeof(struct PowerHeader);
   Hdr->src_svc = src_svc;
   Hdr->dest_svc = POWER_SVC_NUM;
   Hdr->req_type_amt = (REG_NEW_SVC << 4);
   Hdr->pid = getpid();
}


void sig_handler(int sig)
{
  if (SIGALRM == sig)
    running = 0;
  else if (SIGINT == sig)
    testing = 1;
}

void signal_setup(struct sigaction *Sa, log *Log)
{
  sa.sa_handler = sig_handler;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;

  if(-1 == sigaction(SIGINT, &sa, NULL))
    {
      write_errlog(Log, "Couldn't set signal handler for SIGINT");
      return;
    }

  if(-1 == sigaction(SIGALRM, &sa, NULL))
    {
      write_errlog(Log, "Couldn't set signal handler for SIGALRM");
      return;
   } 
}

int req_pwr(conn Pwr, pwr PwrHdr, log *Log)
{
   PwrHdr->ver = 1;
   PwrHdr->len = sizeof(struct PowerHeader);
   PwrHdr->src_svc = SVC_NUM;
   PwrHdr->dest_svc = POWER_SVC_NUM;
   PwrHdr->req_type_amt = (REG_NEW_SVC << 4);
   PwrHdr->pid = getpid();

   if(s_send(Pwr->fd, (void *)PwrHdr, sizeof(struct PowerHeader)) < 0)
   {
     write_errlog(Log, "Communicating to Power Service");
      return -1;
   }
   
   /*WTF??*/
   tmp[0] = '\0';
   if(s_send(Pwr->fd, (void *)tmp, strlen(tmp) + 1) < 0)
   {
     write_errlog(Log, "Terminating registration");
      return -1;
   }
}

int recv_pwr(conn Pwr, pwr PwrHdr)
{
  if(recv(Pwr->remote_sockfd, (void *)PwrHdr, sizeof(struct PowerHeader)) < 0)
    return PWR_ERR_RECV_PWR;
}

void close_pwr()
{

}


int get_pwr_alloc(conn Power, pwr Hdr)
{
 
   /* Power header */
   struct PowerHeader *pwrhd;
   char tmp[1];




   /* Get current power allocation */



   estat.pwr_alloc = pwrhd->alloc;

   free(pwrhd);

   return estat.pwr_alloc;
}


   sa.sa_handler = sig_handler;
   sigfillset(&sa.sa_mask);
   sa.sa_flags = 0;

   if(-1 == sigaction(SIGINT, &sa, NULL))
   {
      perror("Couldn't set signal handler for SIGINT");
      return;
   }

   if(-1 == sigaction(SIGALRM, &sa, NULL))
   {
      perror("Couldn't set signal handler for SIGALRM");
      return;
   }

