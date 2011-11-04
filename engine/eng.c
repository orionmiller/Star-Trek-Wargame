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

#define MAX_IMPL_SPEED 6
#define MAX_WARP_SPEED 9
#define MAX_SHIP_DMG 35
#define DMG_REPAIR_TIME 15
#define DMG_REPAIR_AMT 5
#define DMG_PER_UNIT_RAD 1
#define COOLING_RATE 16

/* Power Usages for Speeds where 0th Index is Impulse */
int eng_pwr_use[10] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

/* File Descriptors for Socketing */
int sockfd;
int httpPt;

/* Control termination of main loop */
volatile sig_atomic_t running = 1;
volatile sig_atomic_t testing = 0;
volatile sig_atomic_t repair_dmg_time = DMG_REPAIR_TIME;

/* Internal Data Storage for Current Status */
typedef struct status_struct Status;

struct status_struct {
   int warp_sp;
   int imp_sp;
   int eng_dmg;
   int pwr_alloc;
   int tot_rad;
   time_t eng_start, eng_stop;
};

Status estat = {0, 0, 0, 0, 0};

/* Heat Generated for impulse speeds using log10(1/16) / log10(imp_actual) */
double imp_rad[] = {1.0000, 1.1158, 1.333, 1.5474, 1.6563, 2.0000};
/*
   Heat Generated for warp speeds using
   - Below Warp 6: (1 + log10(warp) / log10(5)) + 8
   - Warp 6 and Above (e^((warp - 5) / 2) + 15
*/
double warp_rad[] = {8.0000, 11.4454, 13.4608, 14.8908, 16.0000, 16.6487,
                     17.7183, 19.4817, 22.3891};

/* For Mutex Locking */
static pthread_mutexattr_t attr;
static pthread_mutex_t mutex;

int getPwrAlloc(void);
void *request_handler(void *in);
void send_packet(struct EngineHeader *, int);
void req_report(int confd);
void print_engine_status(void);
void assess_damage(void);

int tcpConnect();
connection *sslConnect(void);
void sslDisconnect(connection *c);
void sslWrite(connection *c, void *text, int len);

/*
   Engine Service Function Structure Declaration.

   Initialize to eng.c's associated functions
*/
struct EngineFuncs eng_funcs = {&request_handler, &req_report, NULL};

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
   /* Temp Time */
   time_t tm;
   
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

   /* Setup and Bind STATIC_ENG_PORT */
   bzero(&sck, sizeof(sck));

   /* Set the Address IP_ADDRESS */
   if(inet_pton(AF_INET, IP_ADDRESS, &(sck.sin_addr)) > 0)
   {
      sck.sin_port = STATIC_ENG_PORT;
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

   /* Set pthread mutex attributes and initialize mutex */
   pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&mutex, &attr);

   /* Print to Log File? */
   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp, ": Engine Service at %s Listening on TCP Port %d\n", 
      IP_ADDRESS, httpPt);
   write(logfd, tmp, strlen(tmp));

   /* Setup timers */
   testtv.it_interval.tv_sec = testtv.it_interval.tv_usec = 0;
   resttv.it_interval.tv_sec = resttv.it_interval.tv_usec = 0;
   testtv.it_value.tv_sec = 60;
   testtv.it_value.tv_usec = 0;
   resttv.it_value.tv_sec = resttv.it_value.tv_usec = 0;
  
   /* IF the command line interface IS set THEN */
   if(eng_funcs.cmd_line_inter)
   {
      /* Spawn and new thread and call the function */
      if(pthread_create(&tid,
                        NULL,
                        eng_funcs.cmd_line_inter,
                        NULL) != 0)
         perror("Error Creating new Thread for Command Line Interface");
   }

   estat.eng_stop = -1;

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
         
         /* Quickly run Engine maintenance */
         /* IF damange was done THEN */
         if(!estat.eng_dmg)
            repair_dmg_time--;

         assess_damage();

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
   time_t tm;
   struct sockaddr_in pwr_sck;
   /* Power header */
   char tmp[100];
   tm = time(NULL);
   struct PowerHeader *pwrhd;
   int pwr_sockfd;

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
   time_t tm;
   int rtn;
   char *inBuf;
   char tmp[100];
   ssize_t rt = 1;
   int totalRecv = 0;
   struct EngineHeader *hd;
   struct EngineHeader msg;
   uint8_t req_type;
   uint8_t eng_type;

   pthread_detach(pthread_self());
   inBuf = (char *)malloc(MAXBUF);
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
   
   if(estat.eng_dmg >= MAX_SHIP_DMG && req_type != REQ_REPORT)
   {
      sprintf(tmp, ": Error Setting Engine Speed. Engines are too Damaged\n");
      write(logfd, tmp, strlen(tmp));
   }
   else
   {
      switch(req_type)
      {
         case REQ_INIT_IMPLS_DRV:
            rtn = eng_funcs.wengage_impulse(hd->amt);
            (rtn != -1) ? 
               sprintf(tmp, ": Impulse Engines Started Up & Speed Now %d\n", rtn)
               : 0;
            write(logfd, tmp, strlen(tmp));
            break;
         case REQ_INIT_WARP_DRV:
            rtn = eng_funcs.wengage_warp(hd->amt);
            (rtn != -1) ? 
               sprintf(tmp, ": Warp Engines Started Up & Speed Now %d\n", rtn)
               : 0;
            write(logfd, tmp, strlen(tmp));
            break;
         case REQ_INCR_IMPLS:
            rtn = eng_funcs.wimpulse_speed(hd->amt);
            (rtn != -1) ? 
               sprintf(tmp, ": Impulse Speed Increased to %d\n", rtn) : 0;
            write(logfd, tmp, strlen(tmp));
            break;
         case REQ_DECR_IMPLS:
            rtn = eng_funcs.wimpulse_speed(-1 * hd->amt);
            (rtn != -1) ? 
               sprintf(tmp, ": Impulse Speed Decreased to %d\n", rtn) : 0;
            write(logfd, tmp, strlen(tmp));
            break;
         case REQ_INCR_WARP:
            rtn = eng_funcs.wwarp_speed(hd->amt);
            (rtn != -1) ? 
               sprintf(tmp, ": Warp Speed Increased to %d\n", rtn) : 0;
            write(logfd, tmp, strlen(tmp));
            break;
         case REQ_DECR_WARP:
            rtn = eng_funcs.wwarp_speed(-1 * hd->amt);
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
      /* IF speed <= MAX and enough power */
      if((speed <= MAX_IMPL_SPEED) && 
         eng_pwr_use[0] <= estat.pwr_alloc)
      {         
         /* Set Impulse Speed */
         estat.imp_sp = speed;

         /* Mark Time Engine Start */
         estat.eng_start = time(NULL);

         pthread_mutex_unlock(&mutex);
         /* Return Set Speed */
         return speed;
      }
      else
      {
         pthread_mutex_unlock(&mutex);
         
         sprintf(tmp, ": Invalid Init Impulse Speed %d or Insufficient Power (%d/%d)\n",
            speed, eng_pwr_use[0], estat.pwr_alloc);
         write(logfd, tmp, strlen(tmp));
         
         return -1;
      }
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
      /* IF resulting speed <= MAX and power available THEN */
      if(((estat.imp_sp + speed) <= MAX_IMPL_SPEED) &&
         eng_pwr_use[0] <= estat.pwr_alloc)
      {
         /* Increase impulse speed by speed */
         estat.imp_sp += speed;
         
         /* IF speed now zero THEN */
         if(!estat.imp_sp)
            estat.eng_stop = time(NULL);
         
         pthread_mutex_unlock(&mutex);
         
         return estat.imp_sp;
      }
      else
      {
         pthread_mutex_unlock(&mutex);
         
         sprintf(tmp, ": Invalid Resulting Impulse Speed %d or Insufficient Power (%d/%d)\n",
            estat.imp_sp + speed, eng_pwr_use[0], estat.pwr_alloc);
         write(logfd, tmp, strlen(tmp));

         return -1;
      }
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
      /* IF speed <= MAX and power available THEN */
      if((speed <= MAX_WARP_SPEED) && 
         eng_pwr_use[speed] <= estat.pwr_alloc)
      {
         /* Set Warp Speed */
         estat.warp_sp = speed;

         /* Mark Time Engine Start */
         estat.eng_start = time(NULL);
         
         pthread_mutex_unlock(&mutex);
         
         return speed;
      }
      else
      {
         pthread_mutex_unlock(&mutex);
         
         sprintf(tmp, ": Invalid Init Warp Speed (%d) or Insufficient Power (%d/%d)\n",
            speed, eng_pwr_use[speed], estat.pwr_alloc);
         write(logfd, tmp, strlen(tmp));
         
         return -1;
      }
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
      /* IF speed <= MAX and power available THEN */
      if(((estat.warp_sp + speed) <= MAX_WARP_SPEED) && 
         eng_pwr_use[estat.warp_sp + speed] <= estat.pwr_alloc)
      {
         /* Increase impulse speed by speed */
         estat.warp_sp += speed;
         
         /* IF speed now zero THEN */
         if(!estat.warp_sp)
            estat.eng_stop = time(NULL);
         
         pthread_mutex_unlock(&mutex);
      }
      else
      {
         pthread_mutex_unlock(&mutex);
         sprintf(tmp, ": Invalid resulting Warp speed (%d) or Insufficient Power (%d/%d)\n",
            estat.warp_sp + speed, eng_pwr_use[estat.warp_sp + speed], estat.pwr_alloc);
         write(logfd, tmp, strlen(tmp));
      }
      
      return estat.warp_sp;
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
                        (estat.warp_sp ? WARP_DRIVE : SHUTDOWN_DRIVE)) << 4)
                        | (REQ_REPORT);
   msg.amt = (estat.imp_sp) ? (estat.imp_sp) : 
             ((estat.warp_sp) ? (estat.warp_sp) : 0);
   msg.dmg = estat.eng_dmg;
   msg.rad = estat.tot_rad;
   pthread_mutex_unlock(&mutex);
   
   send_packet(&msg, confd);
}

void assess_damage(void)
{
   time_t tm = time(NULL);
   
   /* IF the engines have been up for ~60s THEN */
   if(estat.eng_start - tm >= 60)
   {
      /* Add Radiation or Damage to Engines */
      estat.tot_rad += (estat.imp_sp ? imp_rad[estat.imp_sp] :
                        (estat.warp_sp ? warp_rad[estat.warp_sp] : 0));
      
      /* Do cooling on Radiation */
      estat.tot_rad -= (estat.tot_rad >= COOLING_RATE) ? 
                        COOLING_RATE : estat.tot_rad;
      
      /* IF radiation remaining THEN */
      if(estat.tot_rad > 0)
      {
         /* Add appropriate damage for radiation remaining */
         estat.eng_dmg += estat.tot_rad % DMG_PER_UNIT_RAD;
      }
      
      /* IF can repair damage THEN */
      if(!repair_dmg_time)
      {
         estat.eng_dmg -= (estat.eng_dmg > DMG_REPAIR_AMT) ?
                           DMG_REPAIR_AMT : estat.eng_dmg;
         repair_dmg_time = DMG_REPAIR_TIME;
      }
      
      /* IF engines have since been shutdown THEN */
      if(estat.eng_stop != -1)
      {
         /* RESET eng_start and stop */
         estat.eng_stop = estat.eng_start = -1;
      }
   }
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

/*
   Test Functions

   RETURN -1 on error, otherwise 0 (or anything not -1)
*/

/* 
   Test warp engines can be initialized properly AND aren't initialized 
      when Impulse or Warp already init'ed
*/
int initWarp_test()
{
   return -1;
}

/*
   Test imp engines can be init'ed properly AND aren't init'ed when
      Warp or Impulse already init'ed
*/
int initImpulse_test()
{
   return -1;
}

/* Test only valid Imp speeds are accepted */
int validImpSpeeds_test()
{
   return -1;
}


/* Test only valid Warp speeds are accpeted */
int validWarpSpeeds_test()
{
   return -1;
}

/* Test set of Warp speed is set correctly (does not go neg or above max) */
int setWarpSpeed_test()
{
   return -1;
}

/* Test set of Imp speed is set correctly (does not go neg or above max) */
int setImpSpeed_test()
{
   return -1;
}

/*
   The main test function to call the other test and report to
      Score Keeper when done.
*/
void run_tests()
{
   
   initWarp_test();
   initImpulse_test();
   validImpSpeeds_test();
   validWarpSpeeds_test();
   setWarpSpeed_test();
   setImpSpeed_test();
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
