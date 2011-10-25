/* The Static Port for Main Navigation Service to run on */
#define STATIC_NAV_PORT 1336

/* Definitions for Possible Directions */
#define CRS_NORTH 0
#define CRS_NORTH_EAST 1
#define CRS_EAST 2
#define CRS_SOUTH_EAST 3
#define CRS_SOUTH 4
#define CRS_SOUTH_WEST 5
#define CRS_WEST 6
#define CRS_NORTH_WEST 7

/*
   NOTES:
      - The Main Navigation service uses and catches SIGINT and SIGALRM. Do not
         use or block these signals or your implementation will not pass
         any of the tests and your team will lose points.
      - The service does nothing with any other signal. Feel free to use
         then if your implementation requires.
*/

/*
   The structure of a Navigation Packet is as follows:

   0           4           8           12          16 
  |--------------------------------------------------|
  |        Version         |    Length of Packet     |
  |--------------------------------------------------|
   17          20          24          28          32
  |--------------------------------------------------|
  |  Eng Type  |   Speed   |        Direction        |
  |--------------------------------------------------|
   33          36          40          44          48
  |--------------------------------------------------|
  |                      Padding                     |
  |--------------------------------------------------|

   Where
      - Engine Type:
         Can be either IMPULSE_DRIVE, WARP_DRIVE, or others if you add
      - Speed:
         The speed index at which to travel (see definitions in eng_packet.h)
      - Direction:
         Can be any of the DEFS listed above prefixed with "CRS_"
      - Duration:
         Is the length of time (in seconds) to travel 
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
struct NavigationHeader {
   uint8_t ver;         /* The Version of the Packet */
   uint8_t len;         /* The length of packet (in bytes) incase you add to it */
   uint8_t eng_type_spd;
   uint8_t dir;
};
