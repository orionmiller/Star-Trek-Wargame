/* The Static Port for Main Engine Service to run on */
#define STATIC_ENG_PORT 3723

/* Definitions for Engine Type */
#define SHUTDWN_DRIVE 0
#define IMPULSE_DRIVE 1
#define WARP_DRIVE 2

/* Definitions for Engine Service Request Types */
#define REQ_INIT_IMPLS_DRV 0
#define REQ_INIT_WARP_DRV 1
#define REQ_INCR_IMPLS 2
#define REQ_DECR_IMPLS 3
#define REQ_INCR_WARP 4
#define REQ_DECR_WARP 5
#define REQ_REPORT 6

/* Definitions for speed indexes */
#define IMP_ALL_STOP 0
#define IMP_QTR 1
#define IMP_THRD 2
#define IMP_HALF 3
#define IMP_TWO_THRD 4
#define IMP_THR_QTR 5
#define IMP_FULL 6

/*
   NOTES:
      - The Main Engine service uses and catches SIGINT and SIGALRM. Do not
         use or block these signals or your implementation will not pass
         any of the tests and your team will lose points.
      - The service does nothing with any other signal. Feel free to use
         then if your implementation requires.
*/

/*
   The structure of a Engine Packet is as follows:

   0           4           8           12          16 
  |--------------------------------------------------|
  |        Version         |    Length of Packet     |
  |--------------------------------------------------|
   17          20          24          28          32
  |--------------------------------------------------|
  |  Eng Type  | Req Type  |         Amount          |
  |--------------------------------------------------|
   33          36          40          44          48
  |--------------------------------------------------|
  |          Damage        |        Radiation        |
  |--------------------------------------------------|

   Where
      - Engine Type:
         Can be either IMPULSE_DRIVE, WARP_DRIVE, or others if you add
      - Request Type:
         Can be either of the #defines above with 'REQ_' prefixes
         REQ_REPORT: Request the current status of the engines. Returns:
            - Which engine is engaged (in Eng Type; SHUTDWN_DRIVE if neither
               engine is on)
            - The current speed (in Amount)
            - 
      - Amount:
         The Unsigned Amount of Speed to increase, decrease, or initialize.
         Use the Definitions above with prefix 'IMP_' for impulse indexing
            speeds. For Warp, simply use the Warp Number (1, 2, ... 9) where
            a Warp of 0 is full stop.
      - Padding
         Whatever
   You can use and modify the following structure to overlay on to incomming 
      data for easier access. For example:

      void function(void *in)
      {
         struct EngineHeader *p;

         p = (struct EngineHeader*)(in);
      }

   This structure is used internally and must NOT be changed or renamed while
      using the provided request_handler function.
*/
struct EngineHeader {
   uint8_t ver;         /* The Version of the Packet */
   uint8_t len;         /* The length of the packet (in bytes) incase you add to it */
   uint8_t eng_type_req; /* The Type of Engine and the Type of Request */
   uint8_t amt;         /* The amount requested or reported (ususlly speed) */
   uint8_t dmg;         /* Damage Inflicted on Engine Core */
   uint8_t rad;         /* Current Level of Radiation in Engine Core */
};
