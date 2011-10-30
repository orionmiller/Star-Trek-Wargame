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

#define SK_IP "10.13.37.4"
#define SK_PORT 1025
#define SCORE_KEEPER_PORT 1234
#define MAX_SHIP_WARP 9
#define MAX_SHIP_DMG 25

typedef struct map_struct MapObject;

struct map_struct
{
   int obj_type;
   int x_pos;
   int y_pos;
   MapObject *next;
};

struct Navigation_Stats
{
   int map_x;              /* The number of grid units in the x direction */
   int map_y;              /* The number of grid units in the y direction */
   int map_dim;            /* The number of Kilometers across a grid */
   int eng_type;           /* The type of engine currently engaged */
   int speed;              /* The current speed index for the type of engine current engaged */
   int ship_dir;           /* The current direction the ship is moving */
   int pos_x, pos_y;       /* The current x and y grid coordinates of the ship's position */
   time_t course_changed;  /* The time the current course settings were made */
   MapObject *map_objs;    /* An array list of objects in the map */
} nav_stats;

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
   struct sockaddr_storage stg;
   socklen_t len;
   int tcpfd;

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
   
   if(getMap() < 0)
   {
      printf("Error getting map from Score Keeper\n");
      return;
   }

   /* Setup and Bind STATIC_PORT */
   bzero(&sck, sizeof(sck));
   
   /* Set the Address IP_ADDRESS */
   if(inet_pton(AF_INET, IP_ADDRESS, &(sck.sin_addr)) > 0)
   {
      sck.sin_port = STATIC_NAV_PORT;
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
   testtv.it_value.tv_sec = 60;
   testtv.it_value.tv_usec = 0;
   resttv.it_value.tv_sec = resttv.it_value.tv_usec = 0;
   
   /* IF the command_line_interface IS setup THEN */
   if(nav_funcs.cmd_line_inter)
   {
      /* Spawn a new thread and call the function */
      if(pthread_create(&tid, 
                        NULL, 
                        nav_funcs.cmd_line_inter, 
                        NULL) != 0)
         perror("Error Creating new Thread for Command Line Interface");
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

int getSKConnection()
{
   int sk_sockfd;
   struct sockaddr_in sk_sck;

   /* Connect to Score Keeper and request Map */
   bzero(&sk_sck, sizeof(sk_sck));
   sk_sck.sin_family = AF_INET;

   if(inet_pton(AF_INET, SK_IP, &(sk_sck.sin_addr)) > 0)
      sk_sck.sin_port = (SK_PORT);
   else
   {
      perror("Bad IP Address");
      return -1;
   }
   
   if((sk_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket Error");
      return -1;
   }

   if(connect(sk_sockfd, (struct sockaddr *)&sk_sck, sizeof(sk_sck)) == -1)
   {
      perror("Score Keeper Connecting Error");
      return -1;
   }

   return sk_sockfd;
}

/* 
   Connect to the Score Keeper and get the Map.

   Request Message:
      <Team Name> request map

   Response Message:
      x:int,y:int,dim:int,px:int,py:int(,<opxt:int,opx:int,opy:int>)*

      Where:
      - x:int is the width of the map (horizontal size)
      - y:int is the length of the map (vertical size)
      - dim:int is the x and y dimention of a grid
      - px:int the x position index for this ship to start in
      - py:int the y position index for this ship to start in
      - opt:int the type of the following object
      - opx:int the x position index of an object (optional)
      - opy:int the y position index of an object (optional)
   
   RETURNS 1 on success or -1 on error.
*/
int getMap()
{
   int sk_sockfd;
   char *tmp;
   char *otok, *itok;
   char **sptr_out = NULL, **sptr_in = NULL;
   MapObject *tmpObj;

   tmp = (char *)malloc(100);
   bzero(&tmp, 100);
   
   /* Connect to Score Keeper and request Map */
   if((sk_sockfd = getSKConnection() == -1))
      return -1;

   sprintf(tmp, "request map\n");
   tmp[strlen(tmp)] = '\0';

   /* Get current map from ScoreKeeper */
   if(write(sk_sockfd, (void *)tmp, strlen(tmp)) < 0)
   {
      perror("Communicating to Score Keeper for Map");
      return -1;
   }
   
   if(read(sk_sockfd, (void *)tmp, 100) < 0)
   {
      perror("Receiving Map from Score Keeper");
      return -1;
   }

   close(sk_sockfd);

   /* Get Map X Width */
   otok = strtok_r(tmp, ",", sptr_out);
   itok = strtok_r(otok, ":", sptr_in);
   itok = strtok_r(NULL, ":", sptr_in);
   nav_stats.map_x = atoi(itok);

   /* Get Map Y Length */
   otok = strtok_r(NULL, ",", sptr_out);
   itok = strtok_r(otok, ":", sptr_in);
   itok = strtok_r(NULL, ":", sptr_in);
   nav_stats.map_y = atoi(itok);

   /* Get Map Grid Demension */
   otok = strtok_r(NULL, ",", sptr_out);
   itok = strtok_r(otok, ":", sptr_in);
   itok = strtok_r(NULL, ",", sptr_in);
   nav_stats.map_dim = atoi(itok);

   /* Get Ship Init X Pos */
   otok = strtok_r(NULL, ",", sptr_out);
   itok = strtok_r(otok, ":", sptr_in);
   itok = strtok_r(NULL, ":", sptr_in);
   nav_stats.pos_x = atoi(itok);

   /* Get Ship Init Y Pos */
   otok = strtok_r(NULL, ",", sptr_out);
   itok = strtok_r(otok, ":", sptr_in);
   itok = strtok_r(NULL, ":", sptr_in);
   nav_stats.pos_y = atoi(itok);

   /* Get Map Objects */
   while((otok = strtok_r(NULL, ",", sptr_out)))
   {
      tmpObj = (MapObject *)malloc(sizeof(MapObject));
      itok = strtok_r(otok, ":", sptr_in);
      itok = strtok_r(NULL, ":", sptr_in);
      tmpObj->obj_type = atoi(itok);
      
      otok = strtok_r(NULL, ",", sptr_out);
      itok = strtok_r(otok, ":", sptr_in);
      itok = strtok_r(NULL, ":", sptr_in);
      tmpObj->x_pos = atoi(itok);

      otok = strtok_r(NULL, ",", sptr_out);
      itok = strtok_r(otok, ":", sptr_in);
      itok = strtok_r(NULL, ":", sptr_in);
      tmpObj->y_pos = atoi(itok);
      
      tmpObj->next = nav_stats.map_objs;
      nav_stats.map_objs = tmpObj;
   }
   
   return 1;
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
         eng_sck.sin_port = (STATIC_ENG_PORT);
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
   double d;
   int num_xgrids, num_ygrids, num_grids;
   int v;
   int t_min;
   int x, y;
   int sk_sockfd;
   char *tmp;

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
      /* Get connection to Score Keeper or FAIL */
      if((sk_sockfd = getSKConnection()) == -1)
      {
         pthread_mutex_unlock(&mutex);
         return -1;
      }
         
      /* IF the speed || direction has changed, then update x and y positions */
      if(nav_stats.speed != speed || nav_stats.ship_dir != crs_dir)
      {
         t_min = (time(NULL) - nav_stats.course_changed) / 60;

         if(nav_stats.eng_type == IMPULSE_DRIVE)
            v = imp_spd_min[nav_stats.speed];
         else
            v = warp_spd_min[nav_stats.speed];

         d = v * t_min;

         /* IF they are at least 80% of the way to the next box THEN */
         if((d / nav_stats.map_dim) >= .80)
         {
            num_grids = d / nav_stats.map_dim;
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
            num_ygrids = y * (num_grids % nav_stats.map_y);
            num_xgrids = x * (num_grids % nav_stats.map_x);

            if(nav_stats.pos_x + num_xgrids >= nav_stats.map_x)
               nav_stats.pos_x = (num_xgrids -= (nav_stats.map_x - 1 - nav_stats.pos_x)) - 1;
            else if(nav_stats.pos_x + num_xgrids < 0)
               nav_stats.pos_x = nav_stats.map_x - (num_xgrids - nav_stats.pos_x);
            else
               nav_stats.pos_x += num_xgrids;
               
            if(nav_stats.pos_y + y >= nav_stats.map_y)
               nav_stats.pos_y = (num_ygrids -= (nav_stats.map_y - 1 - nav_stats.pos_y)) - 1;
            else if(nav_stats.pos_y + num_ygrids < 0)
               nav_stats.pos_y = nav_stats.map_y - (num_ygrids - nav_stats.pos_y);
            else
               nav_stats.pos_y += num_ygrids;
   
            /* Notify Score Keeper About New Position */
            tmp = (char *)malloc(sizeof(char) * 150);
            sprintf(tmp, "map update,posx:%d,posy:%d\n", nav_stats.pos_x, nav_stats.pos_y);
            tmp[strlen(tmp)] = '\0';

            /* Getcurrent map from ScoreKeeper */
            if(write(sk_sockfd, (void *)tmp, strlen(tmp)) < 0)
               perror("Communicating to Score Keeper for Status Update");

            /* Set new course and speed */
            nav_stats.ship_dir = crs_dir;
            nav_stats.eng_type = eng_type;
            nav_stats.speed = speed;
            
            close(sk_sockfd);
            pthread_mutex_unlock(&mutex);
            return 1;
         }
         /* ELSE */
         else
         {
            /* Round down and Keep them in the same box */
            /* Set to new speed and dir */
            nav_stats.ship_dir = crs_dir;
            nav_stats.eng_type = eng_type;
            nav_stats.speed = speed;

            close(sk_sockfd);
            pthread_mutex_unlock(&mutex);
            return 0;
         }
      }
      /* ELSE the speed nor direction changed */
      else
      {
         /* RETURN semi-success state? */
         close(sk_sockfd);
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
   return -1;
}

/* Test you can't go faster or slower than max or min engine speed */
int engineSpeedRange_test()
{
   return -1;
}

/* Test you can't switch engines while another is already running */
int engineSwitch_test()
{
   return -1;
}

int engineInit_test()
{
   return -1;
}

int engineShutdown_test()
{
   return -1;
}

void run_tests()
{
   courseRange_test();
   engineSpeedRange_test();
   engineSwitch_test();
}
