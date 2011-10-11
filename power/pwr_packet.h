/* The static port for the Main Power Service to run on */
#define STATIC_PWR_PORT 1984

/* Service Number Definitions */
#define POWER_SVC_NUM 0
#define TELEPORTER_SVC_NUM 1
#define LIFESUPPORT_SVC_NUM 2
#define COMMS_SVC_NUM 3
#define NAVIGATION_SVC_NUM 4
#define SHIELDS_SVC_NUM 5
#define IMPULSE_ENGINES_SVC_NUM 6
#define WARP_ENGINES_SVC_NUM 7

/* Power Service Request Types */
/* Request De/Allocation of Reserve Power */
#define REQ_RES_ALLOC 0
#define REQ_RES_DEALLOC 1
/* Request Transfer of Allocated Power */
#define REQ_TRANS 2
/* Un/Register a new Service with the Power Service */
#define REG_NEW_SVC 3
#define UNREG_NEW_SVC 4
/* Reporting Current Service Allocation */
#define REG_SVC_ALLOC 5
/* Error On Last Request */
#define REQ_ERROR 6

/*
   NOTES:
      - The Main Power service uses and catches SIGINT and SIGALRM. Do not
         use or block these signals or your implementation will not pass
         any of the tests and your team will lose points.
      - The service does nothing with any other signal. Feel free to use
         then if your implementation requires.
*/

/*
   The structure of a Power Packet is as follows:

   0           4           8           12          16    
  |--------------------------------------------------|
  |        Version          |    Length of Packet    |
  |--------------------------------------------------|
   17          20          24          28          32
  |--------------------------------------------------|
  |     Source Service      |   Destination Service  |
  |--------------------------------------------------|
   33          36          40          44          48
  |--------------------------------------------------|
  | Request Type|  Amount   |       Allocated        |
  |--------------------------------------------------|
   49                                              56
  |--------------------------------------------------|
  |                     Process Id                   |
  |--------------------------------------------------|

   Where
      - Source Service:
         The Service Sending this Packet
         Also used as Source for Removing Power in Transfers
      - Destination Service:
         The Service this Packet is Destine For
         Also used as Destination for Power Transfers
      - Request Type:
         Can be one of the #defines listed above
      - Amount:
         How Much Power to Add or Remove
      - Allocated
         How much the Destination Service has been allocated, or how much the 
            Source Service is currently using
      - Process Id
         The Process Id of Source Service with Registering a new Service

   You can use and modify the following structure to overlay on to incomming 
      data for easier access. For example:

      void function(void *in)
      {
         struct PowerHeader *p;

         p = (struct PowerHeader*)(in);
      }

   This structure is used internally and must NOT be changed or renamed while
      using the provided request_handler function.
*/
struct PowerHeader {
   uint8_t ver;         /* The Version of the Packet */
   uint8_t len;         /* The length of the packet (in bytes) incase you add to it */
   uint8_t src_svc;
   uint8_t dest_svc;
   uint8_t req_type_amt;
   uint8_t alloc;
   uint16_t pid;
};
