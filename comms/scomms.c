
#include "comms.h"

int encryptMsg(uint8_t *);
int decryptMsg(uint8_t *);
void *reqHandler(void *);

int main(int argc, char *argv[])
{
  printf("::COMM LINK ONLINE::\n");
  //  comm_funcs.req_handler = &reqHandler;
  comm_funcs.encrypt = &encryptMsg;
  comm_funcs.decrypt = &decryptMsg;

  return EXIT_SUCCESS;
}

int encryptMsg(uint8_t *data)
{
  return encrypt(data);
}

int decryptMsg(uint8_t *data)
{
  return decrypt(data);
}

void *reqHandler(void *in)
{
  //Your implementation
  return (NULL);
}
