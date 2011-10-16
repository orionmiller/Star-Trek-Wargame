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

#include "eng.h"
#include <time.h>

#define MAX_SHIP_WARP 9
#define MAX_SHIP_DMG 25
#define DMG_PER_UNIT_HEAT 1
#define COOLING_RATE 16
#define IMP_ENG_POWER_USE 4
#define WARP1_POWER 5
#define WARP2_POWER 6
#define WARP3_POWER 7
#define WARP4_POWER 8
#define WARP5_POWER 9
#define WARP6_POWER 10
#define WARP7_POWER 11
#define WARP8_POWER 12
#define WARP9_POWER 13

/* File Descriptors for Socketing */
int sockfd;
int httpPt;

/* Log File Descriptor */
int logfd;

/* Temp Time */
time_t tm;

/* Control termination of main loop */
volatile sig_atomic_t running = 1;
volatile sig_atomic_t testing = 0;

/* Internal Data Storage for Current Status */
typedef struct status_struct Status;

struct status_struct {
   int warp_sp;
   int imp_sp;
   int eng_dmg;
   int pwr_alloc;
   int tot_rad;
};

Status estat = {0, 0, 0, 0, 0};

/* Heat Generated for impulse speeds using log10(1/16) / log10(imp_actual) */
double imp_heat[] = {1.0000, 1.1158, 1.333, 1.5474, 1.6563, 2.0000};
/*
   Heat Generated for warp speeds using
   - Below Warp 6: (1 + log10(warp) / log10(5)) + 8
   - Warp 6 and Above (e^((warp - 5) / 2) + 15
*/
double warp_heat[] = {8.0000, 11.4454, 13.4608, 14.8908, 16.0000, 16.6487,
                     17.7183, 19.4817, 22.3891};

/* For Mutex Locking */
static pthread_mutexattr_t attr;
static pthread_mutex_t mutex;

int getPwrAlloc(void);
void *request_handler(void *in);
void send_packet(struct EngineHeader *, int);
void req_report(int confd);
void print_engine_status(void);

/*
   Engine Service Function Structure Declaration.

   Initialize to eng.c's associated functions
*/
struct EngineFuncs eng_funcs = {&request_handler, &req_report};

void sig_handler(int sig)
{
   if (SIGINT == sig)
      running = 0;
   else if (SIGALRM == sig)
      testing = 1;
}

/*
   Main entry point into the "service". Must do house-keeping before 
      starting the service's logic.
*/
void engine_startup()
{
   /* SIGINT Handler */
   struct sigaction sa;

   /* Stuff for Socketing */
   struct sockaddr_in sck;
   struct sockaddr_storage stg;
   socklen_t len;
   int tcpfd;

   /* For breaking from Accept to start testing */
   struct itimerval testtv, resttv;

   /* Temp char array for printing log message */
   char tmp[100];

   /* For Threading */
   pthread_t tid;

   httpPt = 0;

   /* Setup Log File */
   if ((logfd = open(LOG_FILE_LOCATION, O_WRONLY | O_CREAT | 
                     O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0)
   {
      perror("Problem Opening Log File");
      logfd = STDIN_FILENO;
   }

   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp, ": --- Engine Service Startup ---\n");
   write(logfd, tmp, strlen(tmp));

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

   if(getPwrAlloc() < 0)
   {
      printf("Error getting Power Allocation from Power Service\n");
      return;
   }

   /* Setup and Bind STATIC_PORT */
   bzero(&sck, sizeof(sck));

   /* Set the Address (127.0.0.1) */
   if(inet_pton(AF_INET, "127.0.0.1", &(sck.sin_addr)) > 0)
      sck.sin_port = STATIC_ENG_PORT;
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
   if(listen(sockfd, 15) == -1)
   {
      perror("Listening Error");
      return;
   }

   /* Set pthread mutex attributes and initialize mutex */
   pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&mutex, &attr);

   /* Print to Log File? */
   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp, ": Engine Service at 127.0.0.1 Listening on TCP Port %d\n", 
      httpPt);
   write(logfd, tmp, strlen(tmp));

   /* Setup timers */
   testtv.it_interval.tv_sec = testtv.it_interval.tv_usec = 0;
   resttv.it_interval.tv_sec = resttv.it_interval.tv_usec = 0;
   testtv.it_value.tv_sec = 60;
   testtv.it_value.tv_usec = 0;
   resttv.it_value.tv_sec = resttv.it_value.tv_usec = 0;

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
         setitimer(ITIMER_REAL, &resttv, NULL);

         /* Create new thread to handle / process test functions */

         /* TODO: Orion write & call test functions */

         /* Reinstall testing timer */
         testing = 0;
         setitimer(ITIMER_REAL, &testtv, NULL);
         continue;
      }

      if(!running)
         break;

      if(errno)
         perror("Some Error?");

      errno = 0;

      if(pthread_create(&tid,
                        NULL,
                        eng_funcs.request_handler,
                        (void *)&tcpfd) != 0 && errno != EINTR)
      {
         perror("Error Creating new Thread for Incomming Request");
         continue;
      }
   }
}

/*
   Engine Shutdown method. Should clean up the "system" before exiting
      this program. If there are errors during cleanup, "shutdown" anyways
      by simply returning. Let calling functions handel errors.
*/
void engine_shutdown()
{
   struct sockaddr_in pwr_sck;
   /* Power header */
   char tmp[100];
   tm = time(NULL);
   struct PowerHeader *pwrhd;
   int pwr_sockfd;

   /* Connect to Power Service and register self */
   bzero(&pwr_sck, sizeof(pwr_sck));
   pwr_sck.sin_family = AF_INET;

   if(inet_pton(AF_INET, "127.0.0.1", &(pwr_sck.sin_addr)) > 0)
      pwr_sck.sin_port = (STATIC_PWR_PORT);
   else
   {
      perror("Bad IP Address");
      return;
   }

   if((pwr_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket Error");
      return;
   }

   if(connect(pwr_sockfd, (struct sockaddr *)&pwr_sck, sizeof(pwr_sck)) == -1)
   {
      perror("Power Connecting Error");
      return;
   }

   pwrhd = (struct PowerHeader *)malloc(sizeof(struct PowerHeader));
   pwrhd->ver = 1;
   pwrhd->len = sizeof(struct PowerHeader);
   pwrhd->src_svc = ENGINES_SVC_NUM;
   pwrhd->dest_svc = POWER_SVC_NUM;
   pwrhd->req_type_amt = (UNREG_NEW_SVC << 4);
   pwrhd->pid = getpid();

   write(pwr_sockfd, (void *)pwrhd, sizeof(struct PowerHeader));

   tmp[0] = '\0';

   write(pwr_sockfd, (void *)tmp, 1);

   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp,": --- Engine Service Shutting Down ---\n");
   write(logfd, tmp, strlen(tmp));

   free(pwrhd);

   close(pwr_sockfd);
   close(logfd);
   close(sockfd);
}

/*
   Connect to the Power Service and get the Engine Service's current Power
      Allocation.

   SETS the current power allocation for this service.

   RETURNS the current power allocation for this service, or -1 on error.
*/
int getPwrAlloc()
{
   struct sockaddr_in pwr_sck;
   /* Power header */
   struct PowerHeader *pwrhd;
   char tmp[1];
   int pwr_sockfd;

   /* Connect to Power Service and register self */
   bzero(&pwr_sck, sizeof(pwr_sck));
   pwr_sck.sin_family = AF_INET;

   if(inet_pton(AF_INET, "127.0.0.1", &(pwr_sck.sin_addr)) > 0)
      pwr_sck.sin_port = (STATIC_PWR_PORT);
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
   pwrhd->src_svc = ENGINES_SVC_NUM;
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

   estat.pwr_alloc = pwrhd->alloc;

   free(pwrhd);

   return estat.pwr_alloc;
}

/*
   For receiving incoming data from a socket.

   (this may be the function we make everyone else replace)

   *in should be a poniter to a valid socket file descriptor
*/
void *request_handler(void *in)
{
   int confd = *((int *) in);
   int rtn;
   char inBuf[MAXBUF];
   char tmp[100];
   ssize_t rt = 1;
   int totalRecv = 0;
   struct EngineHeader *hd;
   struct EngineHeader msg;
   uint8_t req_type;
   uint8_t eng_type;

   pthread_detach(pthread_self());
   bzero(inBuf, MAXBUF);

   do
   {
      /* IF received 0 bytes and errno was set THEN */
      if((rt = recv(confd, &inBuf[totalRecv], MAXBUF - totalRecv, 0)) == 0 
         && errno != 0)
      {
         perror("Receive Error");
         close(confd);
         pthread_exit(NULL);
         return (NULL);
      }

      totalRecv += rt;
   } while(rt  > 0 && inBuf[totalRecv - 1] != '\0');

   /* Begin byte stream processing */
   hd = (struct EngineHeader *)(inBuf);
   req_type = (hd->eng_type_req && 0x0F);
   eng_type = (hd->eng_type_req && 0xF0) >> 4;
   
   msg.ver = 1;
   msg.len = sizeof(struct EngineHeader);   
   msg.eng_type_req = (eng_type << 4) | REQ_REPORT;

   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   
   switch(req_type)
   {
      case REQ_INIT_IMPLS_DRV:
         rtn = engage_impulse(hd->amt);
         (rtn != -1) ? 
            sprintf(tmp, ": Impulse Engines Started Up & Speed Now %d\n", rtn)
            : 0;
         write(logfd, tmp, strlen(tmp));
         break;
      case REQ_INIT_WARP_DRV:
         rtn = engage_warp(hd->amt);
         (rtn != -1) ? 
            sprintf(tmp, ": Warp Engines Started Up & Speed Now %d\n", rtn)
            : 0;
         write(logfd, tmp, strlen(tmp));
         break;
      case REQ_INCR_IMPLS:
         rtn = impulse_speed(hd->amt);
         (rtn != -1) ? 
            sprintf(tmp, ": Impulse Speed Increased to %d\n", rtn) : 0;
         write(logfd, tmp, strlen(tmp));
         break;
      case REQ_DECR_IMPLS:
         rtn = impulse_speed(-1 * hd->amt);
         (rtn != -1) ? 
            sprintf(tmp, ": Impulse Speed Decreased to %d\n", rtn) : 0;
         write(logfd, tmp, strlen(tmp));
         break;
      case REQ_INCR_WARP:
         rtn = warp_speed(hd->amt);
         (rtn != -1) ? 
            sprintf(tmp, ": Warp Speed Increased to %d\n", rtn) : 0;
         write(logfd, tmp, strlen(tmp));
         break;
      case REQ_DECR_WARP:
         rtn = warp_speed(-1 * hd->amt);
         (rtn != -1) ? 
            sprintf(tmp, ": Warp Speed Decreased to %d\n", rtn) : 0;
         write(logfd, tmp, strlen(tmp));
         break;
      case REQ_REPORT:
         eng_funcs.request_report(confd);
         sprintf(tmp, ": Request Report Received\n");
         write(logfd, tmp, strlen(tmp));
         break;
      default:
         /* Error State */
         sprintf(tmp, ": Error Processing Packet Request. Dropping Request - ");
         write(logfd, tmp, strlen(tmp));
         write(logfd, inBuf, strlen(inBuf));
         write(logfd, "\n", 1);
         break;
   }

   if(rtn != -1)
   {
      msg.amt = rtn;
      send_packet(&msg, confd);
   }
   
   close(confd);
   pthread_exit(NULL);
   return (NULL);
}

void send_packet(struct EngineHeader *msg, int confd)
{
   write(confd, (void *)msg, sizeof(struct EngineHeader));
}

int engage_impulse(int speed)
{
   char tmp[100];
   
   pthread_mutex_lock(&mutex);
   /* IF the Impulse and Warp Engines NOT already engaged THEN */
   if(!estat.imp_sp && !estat.warp_sp)
   {
      /* Set Impulse Speed */
      estat.imp_sp = speed;

      /* Mark Time Engine Start */

      pthread_mutex_unlock(&mutex);
      /* Return Set Speed */
      return speed;
   }
   else
   {
      pthread_mutex_unlock(&mutex);
      /* Log Error */
      sprintf(tmp, ": Error Engaging Impulse Engines. An Engine is already Started\n");
      write(logfd, tmp, strlen(tmp));
      return -1;
   }
}

int impulse_speed(int speed)
{
   char tmp[100];
   
   pthread_mutex_lock(&mutex);
   /* IF Impulse not already started AND Warp not already engaged THEN */
   if(estat.imp_sp && !estat.warp_sp)
   {
      /* Increase impulse speed by speed */
      estat.imp_sp += speed;
      
      /* IF speed now zero THEN */
      if(!estat.imp_sp)
      {
         
      }
      
      pthread_mutex_unlock(&mutex);
      return estat.imp_sp;
   }
   else
   {
      pthread_mutex_unlock(&mutex);
      /* Log Error */
      sprintf(tmp, ": Error Setting Impulse Speed. Impulse Engines NOT Engaged\n");
      write(logfd, tmp, strlen(tmp));
      return -1;
   }
}

int engage_warp(int speed)
{
   char tmp[100];
   
   pthread_mutex_lock(&mutex);
   /* IF the Impulse and Warp Engines NOT already engaged THEN */
   if(!estat.imp_sp && !estat.warp_sp)
   {
      /* Set Impulse Speed */
      estat.warp_sp = speed;

      /* Mark Time Engine Start */

      pthread_mutex_unlock(&mutex);
      /* Return Set Speed */
      return speed;
   }
   else
   {
      pthread_mutex_unlock(&mutex);
      /* Log Error */
      sprintf(tmp, ": Error Engaging Warp Engines. An Engines is already Started\n");
      write(logfd, tmp, strlen(tmp));
      return -1;
   }
}

int warp_speed(int speed)
{
   char tmp[100];
   
   pthread_mutex_unlock(&mutex);
   /* IF Impulse not already started AND Warp not already engaged THEN */
   if(!estat.imp_sp && estat.warp_sp)
   {
      /* Increase impulse speed by speed */
      estat.warp_sp += speed;
      
      /* IF speed now zero THEN */
      if(!estat.warp_sp)
      {
         
      }
      
      pthread_mutex_unlock(&mutex);
      return estat.imp_sp;
   }
   else
   {
      pthread_mutex_unlock(&mutex);
      /* Log Error */
      sprintf(tmp, ": Error Setting Warp Speed. Warp Engines NOT Engaged\n");
      write(logfd, tmp, strlen(tmp));
      return -1;
   }
}

void req_report(int confd)
{
   struct EngineHeader msg;
   
   msg.ver = 1;
   msg.len = sizeof(struct EngineHeader);
   pthread_mutex_lock(&mutex);
   msg.eng_type_req = ((estat.imp_sp ? IMPULSE_DRIVE : 
                        (estat.warp_sp ? WARP_DRIVE : SHUTDWN_DRIVE)) << 4)
                        | (REQ_REPORT);
   msg.amt = (estat.imp_sp) ? (estat.imp_sp) : 
             ((estat.warp_sp) ? (estat.warp_sp) : 0);
   msg.dmg = estat.eng_dmg;
   msg.rad = estat.tot_rad;
   pthread_mutex_unlock(&mutex);
   
   send_packet(&msg, confd);
}

/* Print out current Engine status */
void print_engine_status()
{
   printf("%s Drive Engaged\n", (estat.imp_sp ? "Impulse" : 
                                (estat.warp_sp ? "Warp" : "No")));
   printf("Currents\n");
   printf("\t- Speed: %d\n", (estat.imp_sp ? estat.imp_sp : 
                             (estat.warp_sp ? estat.warp_sp : 0)));
   printf("\t- Engine Damage: %d\n", estat.eng_dmg);
   printf("\t- Engine Radiation: %d\n", estat.tot_rad);
   printf("\t- Power Allocation: %d\n", estat.pwr_alloc);
   fflush(stdout);
}
