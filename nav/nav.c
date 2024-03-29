/**
Main "Navigation" service. To be compiled before given to contestants. 

Contestants must use and implement "nav.c"

No main function included (the contestants will have this). Replaced with "navigation_startup()".
Therefor, must compile with "gcc -c" option.

Compile as:    gcc -c nav.c -o nav.o -lpthread -Wpacked
  or use the Makefile

@author Dirk Cummings
@author Orion Miller
@version White Hat Star Trek CTF
@version Fall 2011
*/

#include "nav.h"
#include <time.h>

#define SK_IP "10.13.37.15"
#define SK_PORT 48122
#define SK_NAV_UPDATE 22456
#define MAX_SHIP_WARP 9
#define MAX_SHIP_DMG 25
#define GRID_DIM 40500
#define NUM_GRIDS 40

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
char phraser[] = {0xC0, 0x97, 0x55, 0x81, 0xB2, 0xE2, 0x42, 0xF0, 0xB1};

/* Team B */
//char phraser[] = {0x46, 0x17, 0x93, 0xA1, 0x51, 0x50, 0x77, 0x80, 0x37};

/* Testing Values */
int eng_type;
int speed;
int ship_dir;

struct Navigation_Stats
{
   int eng_type;           /* The type of engine currently engaged */
   int speed;              /* The current speed index for the type of engine current engaged */
   int ship_dir;           /* The current direction the ship is moving */
   int pos_x, pos_y;       /* The current x and y grid coordinates of the ship's position */
   time_t course_changed;  /* The time the current course settings were made */
} nav_stats;

typedef struct {
   int socket;
   SSL *sslHandle;
   SSL_CTX *sslContext;
} connection;

/* Engine Speed Conversions (in km/Hour) Using 300,000 m/s as C */
//static uint32_t imp_spd_hr[7] = {0, 67500, 90000, 135000, 180000, 202500, 270000};
//static uint32_t warp_spd_hr[7] = {1080000, 2160000, 3240000, 4320000, 5400000, 
//                                 6480000, 7560000, 8640000, 9720000};
/* Engine Speed Conversions (in km/min) Using 300,000 m/s as C */
static uint32_t imp_spd_min[7] = {0, 1125, 1500, 2250, 3000, 3375, 4500};
static uint32_t warp_spd_min[9] = {18000, 36000, 54000, 72000, 90000, 108000, 126000, 144000, 162000};

/* File Descriptors for Socketing */
int sockfd;
int pwr_sockfd;
int httpPt;

/* Control termination of main loop */
volatile sig_atomic_t running = 1;
volatile sig_atomic_t testing = 0;

/* Internal Navigation Allocation */
int pwr_alloc;

/* Heat Generated for impulse speeds using log10(1/16) / log10(imp_actual) */
double imp_heat[] = {1.0000, 1.1158, 1.333, 1.5474, 1.6563, 2.0000};
/* 
   Heat Generated for warp speeds using 
   - Below Warp 6: (1 + log10(warp) / log10(5)) + 8 
   - Warp 6 and Above (e^((warp - 5) / 2) + 15
*/
double warp_heat[] = {8.0000, 11.4454, 13.4608, 14.8908, 16.0000, 16.6487, 
                     17.7183, 19.4817, 22.3891};


static pthread_mutexattr_t attr;
static pthread_mutex_t mutex;

int getPwrAlloc(void);
int getMap(void);
void *request_handler(void *in);
void *draw_map(void *);
void print_navigation_status(void);
void reset_navs(int, int, int, int, int);
void calculate_new_pos(int *new_x, int *new_y);
void sk_update_pos();

void run_tests();

int tcpConnect();
connection *sslConnect(void);
void sslDisconnect(connection *c);
void sslWrite(connection *c, void *text, int len);

/* 
   Navigation Service Function Structure Declaration.
   
   Initialize to nav.c's associated functions
*/
struct NavigationFuncs nav_funcs = {&request_handler, NULL};

void sig_handler(int sig)
{
   if (SIGINT == sig)
      running = 0;
   else if (SIGALRM == sig)
      testing = 1;
}

/* 
   Main entry point into the "service". Must do house-keeping before starting the 
      service's logic.
*/
void navigation_startup()
{
   /* SIGINT Handler */
   struct sigaction sa;

   /* Stuff for Socketing */
   struct sockaddr_in sck;
   struct sockaddr_in cmdsck;
   struct sockaddr_storage stg;
   socklen_t len;
   int tcpfd, cmdfd;

   /* For breaking from Accept to start testing */
   struct itimerval testtv, resttv;

   /* Temp char array for printing log message */
   char tmp[100];

   /* For Threading */
   pthread_t tid;

   /* Temp Time for Logging */
   time_t tm;
   
   httpPt = 0;

   /* Setup Log File */
   if ((logfd = open(LOG_FILE_LOCATION, O_WRONLY | O_CREAT | O_APPEND, 
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0)
   {
      perror("Problem Opening Log File");
      logfd = STDIN_FILENO;
   }

   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp, ": --- Navigation Service Startup ---\n");
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
   bzero(&cmdsck, sizeof(cmdsck));
   
   /* Set the Address IP_ADDRESS */
   if(inet_pton(AF_INET, IP_ADDRESS, &(sck.sin_addr)) > 0)
   {
      sck.sin_port = htons(STATIC_NAV_PORT);
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
//   sck.sin_port = htons(sck.sin_port);
   httpPt = sck.sin_port;

   /* Start Listening on Port */
   if(listen(sockfd, BACKLOG) == -1)
   {
      perror("Listening Error");
      return;
   }

   /* Setup pthread mutex attributes and initialize mutex */
   pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&mutex, &attr);

   /* Print to Log File? */
   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp, ": Navigation Service at %s Listening on TCP Port %d\n", 
      IP_ADDRESS, httpPt);
   write(logfd, tmp, strlen(tmp));

   /* Setup timers */
   testtv.it_interval.tv_sec = testtv.it_interval.tv_usec = 0;
   resttv.it_interval.tv_sec = resttv.it_interval.tv_usec = 0;
   testtv.it_value.tv_sec = 3;
   testtv.it_value.tv_usec = 0;
   resttv.it_value.tv_sec = resttv.it_value.tv_usec = 0;
   
   /* IF the command_line_interface IS setup THEN */
   if(nav_funcs.cmd_line_inter)
   {
      /* Set the Address IP_ADDRESS */
      if(inet_pton(AF_INET, IP_ADDRESS, &(cmdsck.sin_addr)) > 0)
      {
         cmdsck.sin_port = htons(NAV_CLI_PORT);
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
               /* Spawn a new thread and call the function */
               if(pthread_create(&tid, 
                                 NULL, 
                                 nav_funcs.cmd_line_inter, 
                                 (void *)cmdfd) != 0)
                  perror("Error Creating new Thread for Command Line Interface");
            }
         }
      }
      else
         perror("Bad IP Address");
   }

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
         run_tests();

         /* Reinstall testing timer */
         testing = 0;

         /* Send position update to Score Keeper */
         sk_update_pos();

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
                        nav_funcs.request_handler, 
                        (void *)&tcpfd) != 0 && errno != EINTR)
      {
         perror("Error Creating new Thread for Incomming Request");
         continue;
      }
   }
}

/* 
   Navigation Shutdown method. Should clean up the "system" before exiting 
      this program. If there are errors during cleanup, "shutdown" anyways
      by simply returning. Let calling functions handel errors.
*/
void navigation_shutdown()
{
   time_t tm;
   struct sockaddr_in pwr_sck;
   /* Power header */
   char tmp[100];
   tm = time(NULL);
   struct PowerHeader *pwrhd;
   int pwr_sockfd;

   /* Connect to Power Service and unregister self */
   bzero(&pwr_sck, sizeof(pwr_sck));
   pwr_sck.sin_family = AF_INET;

   if(inet_pton(AF_INET, IP_ADDRESS, &(pwr_sck.sin_addr)) > 0)
      pwr_sck.sin_port = htons(STATIC_PWR_PORT);
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
   pwrhd->src_svc = NAVIGATION_SVC_NUM;
   pwrhd->dest_svc = POWER_SVC_NUM;
   pwrhd->req_type_amt = (UNREG_NEW_SVC << 4);
   pwrhd->pid = getpid();

   write(pwr_sockfd, (void *)pwrhd, sizeof(struct PowerHeader));

   tmp[0] = '\0';

   write(pwr_sockfd, (void *)tmp, 1);

   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   sprintf(tmp,": --- Navigation Service Shutting Down ---\n");
   write(logfd, tmp, strlen(tmp));

   free(pwrhd);

   close(pwr_sockfd);
   close(logfd);
   close(sockfd);
}

/*
   Connect to the Power Service and get the Navigation Service's current Power 
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

   /* Connect to Power Service and register self */
   bzero(&pwr_sck, sizeof(pwr_sck));
   pwr_sck.sin_family = AF_INET;

   if(inet_pton(AF_INET, IP_ADDRESS, &(pwr_sck.sin_addr)) > 0)
      pwr_sck.sin_port = htons(STATIC_PWR_PORT);
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
      return -1;
   }
   
   pwrhd = (struct PowerHeader *)malloc(sizeof(struct PowerHeader));
   pwrhd->ver = 1;
   pwrhd->len = sizeof(struct PowerHeader);
   pwrhd->src_svc = NAVIGATION_SVC_NUM;
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

   pwr_alloc = pwrhd->alloc;

   free(pwrhd);

   return pwr_alloc;
}

/* 
   For receiving incoming data from a socket.
   
   (this may be the function we make everyone else replace)
   
   *in should be a poniter to a valid socket file descriptor
*/
void *request_handler(void *in)
{
   int confd = *((int *) in);
   int rtn, eng_sockfd;
   time_t tm;
   char *inBuf;
   char tmp[100];
   ssize_t rt = 1;
   int totalRecv = 0;
   struct NavigationHeader *hd;
   struct EngineHeader msg;
   uint8_t speed, eng_type, dir;
   struct sockaddr_in eng_sck;
   
   int ol_eng_type = nav_stats.eng_type;
   int ol_spd = nav_stats.speed;
   int ol_ship_dir = nav_stats.ship_dir;
   int ol_pos_x = nav_stats.pos_x, ol_pos_y = nav_stats.pos_y;
   
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
   hd = (struct NavigationHeader *)(inBuf);
   speed = (hd->eng_type_spd && 0x0F);
   eng_type = (hd->eng_type_spd && 0xF0) >> 4;
   dir = hd->dir;
   
   msg.ver = 1;
   msg.len = sizeof(struct EngineHeader);   
   msg.eng_type_req = (eng_type << 4);

   /* Get Engine Request Type */
   if(nav_stats.eng_type != SHUTDOWN_DRIVE)
   {
      if(speed > 0)
      {
         if(eng_type == IMPULSE_DRIVE)
            msg.eng_type_req = (eng_type << 4) | REQ_INCR_IMPLS;
         else
            msg.eng_type_req = (eng_type << 4) | REQ_INCR_WARP;
      }
      else
      {
         if(eng_type == IMPULSE_DRIVE)
            msg.eng_type_req = (eng_type << 4) | REQ_DECR_IMPLS;
         else
           msg.eng_type_req = (eng_type << 4) | REQ_DECR_WARP;
      }
   }
   else if(eng_type == IMPULSE_DRIVE)
       msg.eng_type_req = (eng_type << 4) | REQ_INIT_IMPLS_DRV;
   else
      msg.eng_type_req = (eng_type << 4) | REQ_INIT_WARP_DRV; 
   
   tm = time(NULL);
   write(logfd, ctime(&tm), strlen(ctime(&tm)) - 1);
   
   /* IF the course was set, send updated info to engines */
   if((rtn = nav_funcs.wset_course(dir, eng_type, speed)) != -1)
   {
      /* Connect to Power Service and register self */
      bzero(&eng_sck, sizeof(eng_sck));
      eng_sck.sin_family = AF_INET;

      if(inet_pton(AF_INET, IP_ADDRESS, &(eng_sck.sin_addr)) > 0)
         eng_sck.sin_port = htons(STATIC_PWR_PORT);
      else
      {
         perror("Bad IP Address");
         /* Reset nav settings to previous */
         reset_navs(ol_eng_type, ol_spd, ol_ship_dir, ol_pos_x, ol_pos_y);
         rtn = -1;
      }
      
      if((eng_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      {
         perror("Socket Error");
         reset_navs(ol_eng_type, ol_spd, ol_ship_dir, ol_pos_x, ol_pos_y);
         rtn = -1;
      }

      if(connect(eng_sockfd, (struct sockaddr *)&eng_sck, sizeof(eng_sck)) == -1)
      {
         perror("Engine Connecting Error");
         reset_navs(ol_eng_type, ol_spd, ol_ship_dir, ol_pos_x, ol_pos_y);
         rtn = -1;
      }
     
      /* Send Msg to Engines */
      if(write(eng_sockfd, (void *)&msg, sizeof(struct EngineHeader)) < 0)
      {
         perror("Communicating to Engine Service");
         reset_navs(ol_eng_type, ol_spd, ol_ship_dir, ol_pos_x, ol_pos_y);
         rtn = -1;
      }
   
      tmp[0] = '\0';
      if(write(eng_sockfd, (void *)tmp, strlen(tmp) + 1) < 0)
      {
         perror("Terminating Engine Message");
         reset_navs(ol_eng_type, ol_spd, ol_ship_dir, ol_pos_x, ol_pos_y);
         rtn = -1;
      }  
   }

   close(confd);
   pthread_exit(NULL);
   return (NULL);
}

void reset_navs(int ol_eng_type, int ol_spd, int ol_ship_dir, int ol_pos_x, int ol_pos_y)
{
   pthread_mutex_lock(&mutex);
   nav_stats.eng_type = ol_eng_type;
   nav_stats.speed = ol_spd;
   nav_stats.ship_dir = ol_ship_dir;
   nav_stats.pos_x = ol_pos_x;
   nav_stats.pos_y = ol_pos_y;
   pthread_mutex_unlock(&mutex);

   return;
}

int eng_check(int eng)
{
   return (eng >= SHUTDOWN_DRIVE && eng <= IMPULSE_DRIVE) ? 1 : 0;
}

int course_check(int crs)
{
   return (crs >= CRS_NORTH && crs <= CRS_NORTH_WEST) ? 1 : 0;
}

int speed_check(int eng, int sp)
{
   if(eng == SHUTDOWN_DRIVE)
      return 0;
   else if(eng == IMPULSE_DRIVE)
      return (sp >= 0 && sp < 7) ? 1 : 0;
   else if(eng == WARP_DRIVE)
      return (sp > 0 && sp < 10) ? 1 : 0;
   else
      return 0;
}

int set_course(int crs_dir, int eng_type, int speed)
{
   int i;
   char *tmp;
   connection *c;
   unsigned char *pw;

   if(testing)
   {
      eng_type = eng_type;
      speed = speed;
      ship_dir = crs_dir;
      return 0;
   }

   pthread_mutex_lock(&mutex);
   /* IF can make these changes THEN */
   /* IF the current engine matches the eng_type */
   /* IF the current engine does not match the eng_type then no engine (an initialize) */
   /* IF the crs_dir is valid */
   /* IF the eng_type is valid */
   /* IF the speed is valid */
   if(eng_check(eng_type) &&
      course_check(crs_dir) && 
      (nav_stats.eng_type == eng_type 
         || (!nav_stats.speed) 
         || nav_stats.eng_type == SHUTDOWN_DRIVE) &&
      speed_check(eng_type, speed))
   {
      /* Get connection to Score Keeper */
      c = sslConnect();

      if(c->sslHandle == NULL)
      {
         free(c);
         c = NULL;
      }

      /* IF the speed || direction has changed, then update x and y positions */
      if(nav_stats.speed != speed || nav_stats.ship_dir != crs_dir)
      {
         calculate_new_pos(&(nav_stats.pos_x), &(nav_stats.pos_y));
         /* Set new course and speed */
         nav_stats.ship_dir = crs_dir;
         nav_stats.eng_type = eng_type;
         nav_stats.speed = speed;
            
         pthread_mutex_unlock(&mutex);

         tmp = malloc(13);
         pw = malloc(9);

         pw = memcpy(pw, &phraser, 9);

         for(i = 0; i < 9; i++)
         {
            XOR((pw+i));
            SWAP((pw+i));
            SUB((pw+i));
            FLIP((pw+i));
            tmp[i] = pw[i];
         }

         free(pw);
    
         tmp[9] = TEAM_ID;
         tmp[10] = nav_stats.pos_x;
         tmp[11] = nav_stats.pos_y;
         sslWrite(c, tmp, 13);
         free(tmp);
         sslDisconnect(c);
         return 1;
      }
      /* ELSE the speed nor direction changed */
      else
      {
         /* RETURN semi-success state? */
         sslDisconnect(c);
         pthread_mutex_unlock(&mutex);
         return 0;
      }
   }
   /* ELSE */
   else
   {
      /* RETURN error state */
      pthread_mutex_unlock(&mutex);
      return -1;
   }
}

/*
   Test Functions

   RETURN -1 on failure, otherwise 0 (or something else other than -1)
*/

/* Test you can't set course to less or greater than possible course defines */
int courseRange_test()
{
   eng_type = -1;
   speed = -1;
   ship_dir = -1;

   if(nav_stats.eng_type == WARP_DRIVE || nav_stats.eng_type == IMPULSE_DRIVE)
   {
      return (nav_funcs.wset_course(-4, 
                  (nav_stats.eng_type == WARP_DRIVE) ? WARP_DRIVE : IMPULSE_DRIVE, 
                  1) == -1 && 
               ship_dir == -1 && 
               speed == -1 &&
               eng_type == -1);
   }
   else
      return 0;
}

/* Test you can't go faster or slower than max or min engine speed */
int engineSpeedRange_test()
{
   eng_type = -1;
   speed = -1;
   ship_dir = -1;

   if(nav_stats.eng_type == WARP_DRIVE || nav_stats.eng_type == IMPULSE_DRIVE)
   {
      return (nav_funcs.wset_course(CRS_WEST, 
                  (nav_stats.eng_type == WARP_DRIVE) ? WARP_DRIVE : IMPULSE_DRIVE,
                  10) == -1 && 
               eng_type == -1 &&
               speed == -1 &&
               ship_dir == -1);
   }
   else
      return 0;
}

/* Test you can't switch engines while another is already running */
int engineSwitch_test()
{
   eng_type = -1;
   speed = -1;
   ship_dir = -1;

   if(nav_stats.eng_type == WARP_DRIVE || nav_stats.eng_type == IMPULSE_DRIVE)
   {
      return (nav_funcs.wset_course(CRS_NORTH, 
                (nav_stats.eng_type == WARP_DRIVE) ? IMPULSE_DRIVE : WARP_DRIVE,
                2) == -1 &&
             eng_type == -1 &&
             speed == -1 &&
             ship_dir == -1);
   }
   else
      return 0;
}

/* Get the new X,Y map grid positions and set to new_x new_y inputs */
void calculate_new_pos(int *new_x, int *new_y)
{
   int x, y;
   double d;
   int num_xgrids, num_ygrids, num_grids;
   int v;
   int t_min;

   t_min = (time(NULL) - nav_stats.course_changed) / 60;

   if(nav_stats.eng_type == IMPULSE_DRIVE)
      v = imp_spd_min[nav_stats.speed];
   else
      v = warp_spd_min[nav_stats.speed];

   d = v * t_min;

   /* IF they are at least 80% of the way to the next box THEN */
   if((d / GRID_DIM) >= .80)
   {
      num_grids = d / GRID_DIM;
      /* Round up and Move them to the next box */
      switch(nav_stats.ship_dir)
      {
         case CRS_NORTH:
            y = -1;
            break;
         case CRS_NORTH_EAST:
            x = 1;
            y = -1;
            break;
         case CRS_EAST:
            x = 1;
            break;
         case CRS_SOUTH_EAST:
            x = y = 1;
            break;
         case CRS_SOUTH:
            y = 1;
            break;
         case CRS_SOUTH_WEST:
            y = 1;
            x = -1;
            break;
         case CRS_WEST:
            x = -1;
            break;
         case CRS_NORTH_WEST:
            x = y = -1;
            break;
         default:
            x = y = 0;
            break;
      }

      /* Scale up to the number of grids they moved */
      num_ygrids = y * (num_grids % NUM_GRIDS);
      num_xgrids = x * (num_grids % NUM_GRIDS);

      if(nav_stats.pos_x + num_xgrids >= NUM_GRIDS)
         (*new_x) = (num_xgrids -= (NUM_GRIDS - 1 - nav_stats.pos_x)) - 1;
      else if(nav_stats.pos_x + num_xgrids < 0)
         (*new_x) = NUM_GRIDS - (num_xgrids - nav_stats.pos_x);
      else
         nav_stats.pos_x += num_xgrids;
               
      if(nav_stats.pos_y + y >= NUM_GRIDS)
         (*new_y) = (num_ygrids -= (NUM_GRIDS - 1 - nav_stats.pos_y)) - 1;
      else if(nav_stats.pos_y + num_ygrids < 0)
         (*new_y) = NUM_GRIDS - (num_ygrids - nav_stats.pos_y);
      else
         (*new_y) += num_ygrids;
   }
   else
   {
      (*new_x) = nav_stats.pos_x;
      (*new_y) = nav_stats.pos_y;
   }
}

void sk_update_pos()
{
   connection *c;
   char *tmp = malloc(13);
   int i, x, y;
   unsigned char *pw = malloc(9);
   c = sslConnect();

   if(c->sslHandle == NULL)
   {
      free(tmp);
      free(pw);
      return;
   }
   
   calculate_new_pos(&x, &y);

   pw = memcpy(pw, &phraser, 9);

   for(i = 0; i < 9; i++)
   {
      XOR((pw+i));
      SWAP((pw+i));
      SUB((pw+i));
      FLIP((pw+i));
      tmp[i] = pw[i];
   }

   free(pw);
    
   tmp[9] = TEAM_ID;
   tmp[10] = x;
   tmp[11] = y;
   sslWrite(c, tmp, 13);
   free(tmp);
   sslDisconnect(c);  
}

void run_tests()
{
   connection *c;
   char *tmp = malloc(12);
   int i;
   char *pw = malloc(9);
   pw = memcpy(pw, &phraser, 9);
   
   c = sslConnect();
   
   if(c->sslHandle == NULL)
   {
      free(pw);
      free(tmp);
      free(c);
      return;
   }
   
   if(courseRange_test() ||
      engineSpeedRange_test() ||
      engineSwitch_test())
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
      tmp[11] = 0;
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
   }
   
   tmp[9] = TEAM_ID;
   tmp[10] = NAVIGATION_SVC_NUM;
   sslWrite(c, tmp, 12);
   free(tmp);
   sslDisconnect(c);
}

/* 
   SSL Connection Stuff 

   The following functions (tcpConnect, sslConnect, sslDisconnect, and sslWrite)
      were taken and modified from the solution provided by cyril at 
      SaveTheIons.Com. The original post can be found at
      http://savetheions.com/2010/01/16/quickly-using-openssl-in-c/
*/
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
      server.sin_port = (testing) ? htons(SK_PORT) : htons(SK_NAV_UPDATE);
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
   if(!c)
      return;

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
