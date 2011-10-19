/* The Static Port for Main Navigation Service to run on */
#define STATIC_NAV_PORT 1336

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
  |    |                   |
  |--------------------------------------------------|
   33          36          40          44          48
  |--------------------------------------------------|
  |                  |                |
  |--------------------------------------------------|

   Where
      - 
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
   uint8_t len;         /* The length of packet (in bytes) incase you add to it */
};
