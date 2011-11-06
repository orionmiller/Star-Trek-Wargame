/**
Main "Power" service. To be compiled before given to contestants. 

Contestants must use and implement "ship_power.c"

No main function included (the contestants will have this). Replaced with "power_startup()".
Therefor, must compile with "gcc -c" option.

Compile as:    gcc -c power.c -o pwrd.o -lpthread -Wpacked
  or use the Makefile

@author Dirk Cummings
@author Orion Miller
@version White Hat Star Trek CTF
@version Fall 2011
*/

#include "power.h"
#include <time.h>

#define MAX_SHIP_POWER 75
#define MAX_RESERVE_POWER 35 
#define INIT_POWER_ALLOC 10

#define SK_IP "10.13.37.15"
#define SK_PORT 48122

/* 
   DECRYPTING DEFINES ORDER
   XOR -> SWAP -> SUB -> FLIP
*/
#define SWAP(pass) ((*pass)=(((*pass)&0x0F)<<4) | (((*pass)&0xF0)>>4))
#define XOR(pass) ((*pass)=(*pass)^0x7B)
#define SUB(pass) ((*pass)-=7)
#define FLIP(pass) ((*pass)=~(*pass))

#define TEAM_ID 0

/* Team A */
//char phraser[] = {0xC0, 0x97, 0x55, 0x81, 0xB2, 0xE2, 0x42, 0xF0, 0xB1};

/* Team B */
char phraser[] = {0x46, 0x17, 0x93, 0xA1, 0x51, 0x50, 0x77, 0x80, 0x37};

typedef struct srvcs_struct Service;

struct srvcs_struct 
{
   char *name;
   int id;
   pid_t pid;
   int pwr_alloc;
   Service *next;
};

typedef struct {
   int socket;
   SSL *sslHandle;
   SSL_CTX *sslContext;
} connection;

/* Services List */
Service *servs = NULL;

/* For Testing */
int gamt, gdest, gsrc;

/* File Descriptors for Socketing */
static int sockfd;
int httpPt;

/* Temp Time */
static time_t tm;

/* Sending Packet Flag / Testing Structure */
static int send_packet_test = 0;
struct PowerHeader *cor_hd = NULL;

/* Control termination of main loop */
volatile sig_atomic_t running = 1;
volatile sig_atomic_t testing = 0;

/* For Mutex Locking */
static pthread_mutexattr_t attr;
static pthread_mutex_t mutex;

void *request_handler(void *in);
//void send_packet(struct PowerHeader *, int confd);
int reservePowerRemaining();
void print();

void run_tests();

int tcpConnect();
connection *sslConnect(void);
void sslDisconnect(connection *c);
void sslWrite(connection *c, void *text, int len);

/* 
   Power Service Function Structure Declaration.
   
   Initialize to power.c's associated functions
*/
struct PowerFuncs pow_funcs = {&request_handler, NULL};

void sig_handler(int sig)
{
   if (SIGINT == sig)
      running = 0;
   else if(SIGALRM == sig)
      testing = 1; 
}

/* 
   Main entry point into the "service". Must do house-keeping before starting the 
      service's logic.
*/
void power_startup()
{
   /* SIGINT Handler */
   struct sigaction sa;

   /* Stuff for Socketing */
   struct sockaddr_in sck;
   struct sockaddr_storage stg;
   struct sockaddr_in cmdsck;

   socklen_t len;
   int tcpfd, cmdfd;

   /* For Breaking from Accept to run Power Unit Tests */
   struct itimerval test_tm, test_rest;

   char tmp[100];

   /* For Threading */
   pthread_t tid;
   
   httpPt = 0;

   /* Setup Log File */
   if ((logfd = open(LOG_FILE_LOCATION, 
                     O_WRONLY | O_CREAT | O_APPEND, 
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0)
   {
      perror("Problem Opening Log File");
      logfd = STDIN_FILENO;
   }

   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   write(logfd, ": --- Power Service Startup ---\n", 32);

   /* Install SignHandler */
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

   /* Setup and Bind STATIC_PWR_PORT */
   bzero(&sck, sizeof(sck));
   bzero(&cmdsck, sizeof(sck));

   /* Set the Address IP_ADDRESS */
   if(inet_pton(AF_INET, IP_ADDRESS, &(sck.sin_addr)) > 0)
   {
      sck.sin_port = htons(STATIC_PWR_PORT);
      sck.sin_family = AF_INET;
   }
   else
   {
      perror("Bad IP Address");
      return;
   }

   /* Get Socket FD from Kernel */
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket Error");
      return;
   }

   /* Bind Socket */
   if(bind(sockfd, (struct sockaddr *)&sck, sizeof(sck)) != 0)
   {
      perror("Socket Binding Error");
      return;
   }

   /* Get the Service port assigned by kernal */
   len = sizeof(sck);
   getsockname(sockfd, (struct sockaddr *)&sck, &len);
   sck.sin_port = htons(sck.sin_port);
   httpPt = sck.sin_port;

   /* Start Listening on Port */
   if(listen(sockfd, BACKLOG) == -1)
   {
      perror("Listening Error");
      return;
   }
   
   pthread_mutex_init(&mutex, &attr);

   /* Print to Log File? */
   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp, ": Power Service at %s Listening on TCP Port %d\n",
      IP_ADDRESS, httpPt);
   write(logfd, tmp, strlen(tmp));

   /* Set up timers */
   test_tm.it_interval.tv_sec = test_tm.it_interval.tv_usec = 0;
   test_rest.it_interval.tv_sec = test_rest.it_interval.tv_usec = 0;
   test_tm.it_value.tv_sec = 60;
   test_tm.it_value.tv_usec = 0;
   test_rest.it_value.tv_sec = test_rest.it_value.tv_usec = 0;

   /* IF the command line interface IS setup THEN */
   if(pow_funcs.cmd_line_inter)
   {
      /* Setup and Bind STATIC_PWR_PORT */
      bzero(&cmdsck, sizeof(cmdsck));
   
      /* Set the Address IP_ADDRESS */
      if(inet_pton(AF_INET, IP_ADDRESS, &(cmdsck.sin_addr)) > 0)
      {
         cmdsck.sin_port = htons(PWR_CLI_PORT);
         cmdsck.sin_family = AF_INET;
         
         /* Get Socket FD from Kernel */
         if((cmdfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            perror("Socket Error");
         else
         {
            /* Bind Socket */
            if(bind(cmdfd, (struct sockaddr *)&cmdsck, sizeof(cmdsck)) != 0)
               perror("Socket Binding Error");
            else
            {
               if(pthread_create(&tid,
                                 NULL,
                                 pow_funcs.cmd_line_inter,
                                 (void *)&cmdfd) != 0)
                  perror("Error Creating new Thread for Command Line Interface");
            }
         }
      }
      else
         perror("Bad IP Address");
   }

   /* Install timer */
   setitimer(ITIMER_REAL, &test_tm, NULL);

   /* Infinite loop for prompt? */
   while(running)
   {
      len = sizeof(stg);
      errno = 0;
      tcpfd = accept(sockfd, (struct sockaddr *)&stg, &len);

      if((errno == EAGAIN || errno == EWOULDBLOCK) && !testing)
      {
         errno = 0;
         continue;
      }
      else if(testing)
      {
         /* Install reset timer */
         setitimer(ITIMER_REAL, &test_rest, NULL);
         /* Create new thread to handle / process test functions */

         /* TODO: Orion write & call test functions */
         run_tests();

         /* Reinstall timer and continue */
         setitimer(ITIMER_REAL, &test_tm, NULL);
         testing = 0;
         continue;
      }

      if(!running)
         break;

      if(errno)
         perror("Some Error?");
 
      errno = 0;

      if(pthread_create(&tid, 
                        NULL, 
                        pow_funcs.request_handler, 
                        (void *)&tcpfd) != 0 && errno != EINTR)
      {
         perror("Error Creating new Thread for Incomming Request");
         continue;
      }
   }
}

/* 
   Power Shutdown method. Should clean up the "system" before exiting 
      this program. If there are errors during cleanup, "shutdown" anyways
      by simply returning. Let calling functions handel errors.
*/
void power_shutdown()
{
   Service *tmp, *ptr;
   tm = time(NULL);

   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   write(logfd, ": --- Power Service Shutting Down ---\n", 38);
   
   /* Wait for all Threads to complete? */
   
   /* Contact all Services to Shutdown */
   for(tmp = servs; tmp; tmp = tmp->next)
   {
      printf("%d\t%d\n", tmp->id, tmp->pid);
      /* TODO: SIGKILL OR SIGSTOP ?*/
      kill(tmp->pid, SIGKILL);
   }
   
   close(logfd);
   close(sockfd);

   ptr = servs;
   while(ptr)
   {
      tmp = ptr->next;
      free(ptr);
      ptr = tmp;
   }
   
   pthread_mutex_destroy(&mutex);
}

/* 
   For receiving incoming data from a socket.
   
   (this may be the function we make everyone else replace)
   
   *in should be a poniter to a valid socket file descriptor
*/
void *request_handler(void *in)
{
   int confd = *((int *) in);
   char *inBuf;
   char tmp[100];
   ssize_t rt = 1;
   int totalRecv = 0;
   struct PowerHeader *pwrhd;
   struct PowerHeader msg;
   uint8_t req_type, amt;
   int svc_id;

   Service *ptr = NULL, *src, *dest;

   pthread_detach(pthread_self());
   inBuf = (char *)malloc(MAXBUF);
   bzero(inBuf, MAXBUF);
   
   msg.ver = 1;
   msg.len = sizeof(struct PowerHeader);
   msg.src_svc = POWER_SVC_NUM;
   
   tm = time(NULL);
   
   do
   {
      /* IF received 0 bytes and errno was set THEN */
      if((rt = recv(confd, &inBuf[totalRecv], MAXBUF - totalRecv, 0)) <= 0 && errno != 0)
      {
         perror("Receive Error");
         close(confd);
         pthread_exit(NULL);
         return (NULL);
      }

      totalRecv += rt;
   } while(rt > 0 && inBuf[totalRecv - 1] != '\0');

   /* Begin Byte Stream Processing */
   pwrhd = (struct PowerHeader *)(inBuf);
   req_type = (pwrhd->req_type_amt & 0xF0) >> 4;
   amt = pwrhd->req_type_amt & 0x0F;

   for(ptr = servs; ptr; ptr = ptr->next)
   {
      if(ptr->id == pwrhd->src_svc)
         src = ptr;
      if(ptr->id == pwrhd->dest_svc)
         dest = ptr;
   }
   
   if((!src || !dest) && (pwrhd->dest_svc != POWER_SVC_NUM))
   {
      /* Error --> DUMP & Report */
      write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
      sprintf(tmp, ": Error Processing Packet. Source (%d) or Destination (%d) are invalid - ",
         pwrhd->src_svc, pwrhd->dest_svc);
      write(logfd, tmp, strlen(tmp));
      write(logfd, inBuf, strlen(inBuf));
      write(logfd, "\n", 1);
   }
   else if(req_type == REQ_TRANS)
   {
      amt = pow_funcs.wtransfer_power(amt, pwrhd->src_svc, pwrhd->dest_svc);
      
      msg.req_type_amt = (REG_SVC_ALLOC << 4);
      msg.dest_svc = dest->id;
      msg.alloc = dest->pwr_alloc;
      
      send_packet(&msg, confd);
   }
   else if(req_type == REQ_RES_ALLOC)
   {
      amt = pow_funcs.wadd_power(amt, pwrhd->dest_svc);
      
      msg.req_type_amt = (REG_SVC_ALLOC << 4);
      msg.dest_svc = dest->id;
      msg.alloc = dest->pwr_alloc;
      
      send_packet(&msg, confd);
   }
   else if(req_type == REQ_RES_DEALLOC)
   {
     amt = pow_funcs.wfree_power(amt, pwrhd->dest_svc);
      
      msg.req_type_amt = (REG_SVC_ALLOC << 4);
      msg.dest_svc = dest->id;
      msg.alloc = dest->pwr_alloc;
      
      send_packet(&msg, confd);
   }
   else if(req_type == REG_NEW_SVC)
   {
      if((svc_id = pow_funcs.wregister_service(pwrhd->src_svc, pwrhd->pid)))
      {
         for(src = servs; src->id != pwrhd->src_svc; src = src->next)
            ;

         write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
         sprintf(tmp, ": New Service With Id %d Now Registered and Upped with %d Allocated\n", 
            src->id, src->pwr_alloc);
         write(logfd, tmp, strlen(tmp));
      }
      else
      {
         for(src = servs; src->id != pwrhd->src_svc; src = src->next)
            ;

         write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
         sprintf(tmp, ": Service (%d) Already Registered\n", src->id);
         write(logfd, tmp, strlen(tmp));
      }         
   
      /* Send Update to pwrhd->dest_svc */
      msg.dest_svc = pwrhd->src_svc;
      msg.req_type_amt = (REG_SVC_ALLOC << 4);
      msg.alloc = src->pwr_alloc;

      send_packet(&msg, confd);
   }
   else if(req_type == UNREG_NEW_SVC)
   {
      if(pow_funcs.wunregister_service(pwrhd->src_svc))
      {
         write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
         sprintf(tmp, ": Service With Id %d Unregistered and Downed\n", pwrhd->src_svc);
         write(logfd, tmp, strlen(tmp));
      }
   }
   else
   {
      /* Error --> DUMP & Report */
      write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
      sprintf(tmp, ": Error Processing Request Packet - - ");
      write(logfd, tmp, strlen(tmp));
      write(logfd, inBuf, strlen(inBuf));
      
      msg.dest_svc = pwrhd->src_svc;
      msg.req_type_amt = (REQ_ERROR << 4);
      
      send_packet(&msg, confd);
   }
   
   close(confd);
   pthread_exit(NULL);
   return (NULL);
}

/*
   Internal Function to send a Power Header Message to confd.
*/
int send_packet(struct PowerHeader *msg, int confd)
{
   if(!testing)
      return (write(confd, (void *)msg, sizeof(struct PowerHeader)) != -1) ? 1 : -1;
   else
   {
      /* Check msg equals cor_hd */
      if(cor_hd->len == msg->len &&
         cor_hd->src_svc == msg->src_svc &&
         cor_hd->dest_svc == msg->dest_svc &&
         cor_hd->req_type_amt == msg->req_type_amt &&
         cor_hd->alloc == msg->alloc &&
         cor_hd->pid == msg->pid)
         send_packet_test = 1;
      else
         send_packet_test = 0;
      
      return 0;
   }
}

/* 
   Send 'amt' of power from 'src' to 'dest'
   Check to make sure there is enough 'amt' power in 'src' before transfer
   If result from minus 'src' is negative, add what is available to 'dest'
*/
int transfer_power(int amt, int src, int dest)
{
   Service *s, *d, *ptr;
   char tmp[100];
   
   if(testing)
   {
      gamt = amt;
      gsrc = src;
      gdest = dest;
      return 1;
   }
   
   tm = time(NULL);
   
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   
   if(amt <= 0)
   {
      sprintf(tmp, ": Invalid Amount of Power %d\n", amt);
      write(logfd, tmp, strlen(tmp));
      return -1;
   }

   pthread_mutex_lock(&mutex);
   /* Get the Source and Dest pointers */
   for(ptr = servs; ptr; ptr = ptr->next)
   {
      if(ptr->id == src)
         s = ptr;
      else if(ptr->id == dest)
         d = ptr;
   }

   if(!s || !d)
   {
      pthread_mutex_unlock(&mutex);
      /* Invalid Source or Dest Service Supplied */
      sprintf(tmp, ": Invalid Source (%d) or Destination (%d) Ids. May not have been registered?\n", src, dest);
      write(logfd, tmp, strlen(tmp));
      
      return -1;
   }
   /* ELSE IF there is enough RESERVE Power THEN */
   else if(reservePowerRemaining() >= amt)
   {
      /* Increase Allocation for Dest and Don't Touch Src */
      d->pwr_alloc += amt;
      
      pthread_mutex_unlock(&mutex);

      sprintf(tmp, ": %d Additional Units of Power Allocated to Service %d\n",
         amt, src);
      write(logfd, tmp, strlen(tmp));      
   }
   else
   {
      /* SUB 'AMT' from 'SRC''s Allocation and ADD 'AMT' to 'DEST''s Alloc */
      amt = (s->pwr_alloc - amt >= 0) ? amt : s->pwr_alloc;
      d->pwr_alloc += amt;
      s->pwr_alloc -= amt;
      
      sprintf(tmp, ": Added %d Units of Power to Destination Service %d\n",
         amt, d->id);
      write(logfd, tmp, strlen(tmp));
      
      // IF Source's Power Allocation is now 0 (Zero) THEN
      if(s->pwr_alloc <= 0)
      {
         // Send SIGSTOP To Source's Process
         if(!testing)
            kill(s->pid, SIGSTOP);
         
         tm = time(NULL);
         write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
         sprintf(tmp, ": Source Service %d has lost ALL Power and is now Stopped\n",
            src);
         write(logfd, tmp, strlen(tmp));
      }
   }
   
   return amt;
}

int add_power(int amt, int dest)
{
   Service *d;
   char tmp[100];
   int rp;

   if(testing)
   {
      gamt = amt;
      gdest = amt;
      return 1;
   }

   tm = time(NULL);
   
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   
   pthread_mutex_lock(&mutex);
   for(d = servs; d && d->id != dest; d = d->next)
      ;

   /* IF the destination does NOT exist THEN */
   if(!d)
   {
      /* Invalid Source or Dest Service Supplied */
      sprintf(tmp, ": Invalid Destination (%d) Id. May not have been registered?\n", dest);
      write(logfd, tmp, strlen(tmp));
      pthread_mutex_unlock(&mutex);
      
      return -1;
   }
  
   /* IF there is reserve power to add THEN */
   if((rp = reservePowerRemaining()))
   {
      /* IF Destination's Allocation is Currently 0 THEN */
      if(d->pwr_alloc <= 0)
      {
         /* Send SIGCONT to Destination's Process Id */
         if(!testing)
            kill(d->pid, SIGCONT);
         
         sprintf(tmp, ": Destination Service %d has lost ALL Power and is now Stopped\n",
            dest);
         write(logfd, tmp, strlen(tmp));
      }
      
      amt = (rp - amt >= 0) ? amt : rp;
      d->pwr_alloc += amt;
      
      pthread_mutex_unlock(&mutex);
      
      tm = time(NULL);
      write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
      sprintf(tmp, ": Added %d Units of Power to Service %d Allocation\n",
         amt, dest);
      write(logfd, tmp, strlen(tmp));
   
      return amt;
   }
   else
   {
      pthread_mutex_unlock(&mutex);
      amt = 0;
      sprintf(tmp, ": Added %d Units of Power to Service %d Allocation\n",
         amt, dest);
      write(logfd, tmp, strlen(tmp));
      return amt;
   }
}

/*
   Remove 'amt' of power from 'dest's Allocation
   Check to make sure 'amt' will not result in a negative amount in 'src'
   If result is negative, take extra power ammount and add back to 'cur_avail_power'
*/
int free_power(int amt, int dest)
{
   char tmp[100];
   Service *d;

   if(testing)
   {
      gamt = amt;
      gdest = dest;
      return 1;
   }

   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   
   pthread_mutex_lock(&mutex);
   
   for(d = servs; d && d->id != dest; d = d->next)
      ;
   
   if(!d)
   {
      /* Invalid Source or Dest Service Supplied */
      sprintf(tmp, ": Invalid Destination (%d) Id. May not have been registered?\n", 
         dest);
      write(logfd, tmp, strlen(tmp));
      pthread_mutex_unlock(&mutex);
      
      return -1;
   }

   amt = (d->pwr_alloc - amt >= 0) ? amt : d->pwr_alloc;
   d->pwr_alloc -= amt;
   
   pthread_mutex_unlock(&mutex);
   
   /* IF Destination's Allocation is <= 0 THEN */
   if(d->pwr_alloc <= 0)
   {
      /* Send SIGSTOP to Destination's Process Id */
      kill(d->pid, SIGSTOP);
      
      sprintf(tmp, ": Destination Service %d has lost ALL Power and is now Stopped\n",
         dest);
      write(logfd, tmp, strlen(tmp));
   }
   else
   {
      sprintf(tmp, ": Service %d current power Allocation %d\n",
         dest, d->pwr_alloc);
      write(logfd, tmp, strlen(tmp));
   }
   
   return amt;
}

int register_service(int id, int pId)
{
   Service *ptr;
   int resPwr = reservePowerRemaining();

   /* FIND a reg'ed service with the same PId, OR the last in the list */
   for(ptr = servs; ptr; ptr = ptr->next)
   {
      if(ptr->id == id || ptr->pid == pId)
         return 0;
      else if(!ptr->next)
         break;
   }
   
   pthread_mutex_lock(&mutex);

   /* IF there are no current Services Registered */
   if(!servs)
   {
      ptr = (Service *)malloc(sizeof(struct srvcs_struct));
      servs = ptr;
   }
   else
   {
      ptr->next = (Service *)malloc(sizeof(struct srvcs_struct));
      ptr = ptr->next;
   }

   bzero(ptr, sizeof(struct srvcs_struct));
   ptr->id = id;
   ptr->pid = pId;
   ptr->pwr_alloc = ((resPwr - INIT_POWER_ALLOC) >= 0) ? INIT_POWER_ALLOC : INIT_POWER_ALLOC - resPwr;
   pthread_mutex_unlock(&mutex);
   return ptr->pwr_alloc;
}

int unregister_service(int id)
{
   Service *ptr, *prev;
   
   pthread_mutex_lock(&mutex);
   for(ptr = prev = servs; ptr; ptr = ptr->next)
   {
      if(ptr->id == id)
      {
         /* IF this is the FIRST in the list THEN */
         if(ptr == prev && ptr == servs)
         {
            servs = ptr->next;
            free(ptr);
         }
         /* IF this is at the END of the list THEN */
         else if(ptr->next == NULL && ptr != prev)
         {
            prev->next = NULL;
            free(ptr);
         }
         /* IF this IS the last service in the list THEN */
         else if(ptr->next == NULL && ptr == prev)
         {
            free(ptr);
            servs = NULL;
         }
         else
         {
            prev->next = ptr->next;
            free(ptr);
         }
         
         pthread_mutex_unlock(&mutex);
         
         return id;
      }
      else
         prev = ptr;
   }
   pthread_mutex_unlock(&mutex);
   return 0;
}

/* Print out current usages for all services */
void print_power_status()
{
   Service *ptr;
   
   for(ptr = servs; ptr; ptr = ptr->next)
      fprintf(stdout, "Service %d:\t%d\n", 
         ptr->id, ptr->pwr_alloc);
         
   fflush(stdout);
}

/*
   Returns the Amount of Power not currently allocated
*/
int reservePowerRemaining()
{
   int curUse = 0;
   Service *ptr = NULL;
   
   pthread_mutex_lock(&mutex);
   for(ptr = servs; ptr; ptr = ptr->next)
      curUse += ptr->pwr_alloc;
      
   pthread_mutex_unlock(&mutex);
   return MAX_SHIP_POWER - curUse;
}

/* 
   Test Functions 

   RETURN -1 if FAILED, otherwise 0 (or anything not -1)
*/

int getNumberServices()
{
   int n = 0;
   Service *ptr;
   
   pthread_mutex_lock(&mutex);
   ptr = servs;
   while(ptr)
   {
      n++;
      ptr = ptr->next;
   }
   
   pthread_mutex_unlock(&mutex);
   
   return n;
}

/* Make sure you can't add more than remaining power available Ship power */
int addTooMuchPower_test()
{
   /* Store old values */
   int num_servs = getNumberServices();
   int cur_pwr_remain = reservePowerRemaining();
   
   gamt = gdest = -1;

   num_servs = pow_funcs.wadd_power(cur_pwr_remain + 10, (num_servs > 0) ? servs->id : 6);

   return (gamt == -1 && gdest == -1 && num_servs == -1);
}

/* Make sure you can't free so much power that the current allocated goes 
   below 0 */
int freeTooMuchPower_test()
{
   /* Store old values */
   int num_servs = getNumberServices();
   int cur_pwr_remain = reservePowerRemaining();
   
   gamt = gdest = -1;

   num_servs = pow_funcs.wfree_power(
                  (num_servs > 0) ? servs->pwr_alloc : 0 + cur_pwr_remain + 1, 
                  (num_servs > 0) ? servs->id : 4);
   
   return (num_servs == -1 && gamt == -1 && gdest == -1);
}

/* Try to transfer more power than source service has available */
int transferTooMuchPower_test()
{
   /* Store old values */
   int num_servs = getNumberServices();
   int cur_pwr_remain = reservePowerRemaining();
  
   gamt = gdest = gsrc = -1;

   if(num_servs > 0)
      num_servs = pow_funcs.wtransfer_power(
                     servs->pwr_alloc + (num_servs < 2) ? 0 : cur_pwr_remain + 1,
                     servs->id, 
                     (num_servs < 2) ? (servs->id + 1) : servs->next->id);
   else
      num_servs = pow_funcs.wtransfer_power(cur_pwr_remain + 1, 2, 1);

   return (num_servs == -1 && gamt == -1 && gdest == -1 && gsrc == -1);
}


/* 
   The main test function to call the other tests and report to 
      Score Keeper when done.
*/
void run_tests()
{
   connection *c;
   char *tmp = malloc(12 + 1);
   int i;
   unsigned char *pw = malloc(9);
   pw = memcpy(pw, &phraser, 9);
   
   bzero(tmp, 12 + 1);

   c = sslConnect();

   if(c->sslHandle == NULL)
   {
      free(c);
      free(pw);
      free(tmp);
      return;
   }

   if(!addTooMuchPower_test() ||
      !freeTooMuchPower_test() ||
      !transferTooMuchPower_test())
   {
      for(i = 0; i < 9; i++)
      {
         XOR((pw+i));
         SWAP((pw+i));
         SUB((pw+i));
         FLIP((pw+i));
         tmp[i] = pw[i];
      }
      free(pw);
      tmp[i] = 0;

      fprintf(stderr, "failed tests\n");
   }
   else
   {
      for(i = 0; i < 9; i++)
      {
         XOR((pw+i));
         SWAP((pw+i));
         SUB((pw+i));
         FLIP((pw+i));
         tmp[i] = pw[i];
      }
      free(pw);
      tmp[11] = 1;

      fprintf(stderr, "passed tests\n");
   }

   tmp[9] = TEAM_ID;
   tmp[10] = POWER_SVC_NUM;

   sslWrite(c, tmp, 12);
   free(tmp);  
   sslDisconnect(c);
}

/* SSL Connection Stuff */
int tcpConnect()
{
   int error, handle;
   struct sockaddr_in server;

   if((handle = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Score Keeper Socket");
      handle = 0;
   }
   else
   {
      server.sin_family = AF_INET;
      server.sin_port = htons(SK_PORT);
      inet_pton(AF_INET, SK_IP, &(server.sin_addr));
      bzero(&(server.sin_zero), 8);

      if((error = connect(handle, (struct sockaddr *) &server, sizeof(struct sockaddr))) == -1)
      {
         perror("Score Keeper Connecting");
         handle = 0;
      }
   }

   return handle;
}

connection *sslConnect(void)
{
   connection *c;

   c = malloc(sizeof(connection));
   c->sslHandle = NULL;
   c->sslContext = NULL;

   if((c->socket = tcpConnect()))
   {
      SSL_load_error_strings();
      SSL_library_init();

      c->sslContext = SSL_CTX_new(SSLv23_client_method());
      
      if(c->sslContext == NULL)
         ERR_print_errors_fp(stderr);

      c->sslHandle = SSL_new(c->sslContext);
      if(c->sslHandle == NULL)
         ERR_print_errors_fp(stderr);

      if(!SSL_set_fd(c->sslHandle, c->socket))
         ERR_print_errors_fp(stderr);

      if(SSL_connect(c->sslHandle) != 1)
         ERR_print_errors_fp(stderr);
   }
   else
      perror("Connect failed");

   return c;
}

void sslDisconnect(connection *c)
{
   if(c->socket)
      close(c->socket);
   
   if(c->sslHandle)
   {
      SSL_shutdown(c->sslHandle);
      SSL_free(c->sslHandle);
   }

   if(c->sslContext)
      SSL_CTX_free(c->sslContext);

   free(c);
}

void sslWrite(connection *c, void *text, int len)
{
   if(c)
      SSL_write(c->sslHandle, text, len);
}
