
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XOR(pass) ((*pass)=(*pass)^0x7B)
#define OR(pass,high,low) ((*pass)=(*high)|(*low))
#define ADD(pass) ((*pass)+=7)
#define SUB(pass) ((*pass)-=7)
#define FLIP(pass) ((*pass)=~(*pass))
#define HIGH_MASK (0xF0)
#define LOW_MASK (0x0F)
#define LOW(pass, low) ((*low)=((*pass)&HIGH_MASK)>>4)
#define HIGH(pass, high) ((*high)=((*pass)&LOW_MASK)<<4)

void print(unsigned char *s, int len)
{
  int c;
  for (c=0; c < len; c++)
    {
      printf("%X ",s[c]);
    }
}

int main(void)
{
  int x;
  int len;
  unsigned char pass[] = "K8$WjmsNZ";
  unsigned char high_t;
  unsigned char low_t;
  len = strlen(pass);
  
  printf("plaintext pass: \"");
  print(pass, len);
  printf("\"\n");
  for (x = 0; x < len; x++)
    {
      high_t = 0;
      low_t = 0;
      FLIP((pass+x));
      ADD((pass+x));
      HIGH((pass+x),(&high_t));
      LOW((pass+x), (&low_t));
      OR((pass+x), (&high_t), (&low_t));
      XOR((pass+x));
    }

  printf("encrypted pass: \"");
  print(pass, len);
  printf("\"\n");
  for (x = 0; x < len; x++)
    {
      high_t = 0;
      low_t = 0;
      XOR((pass+x));
      HIGH((pass+x),(&high_t));
      LOW((pass+x),(&low_t));
      OR((pass+x), (&high_t), (&low_t));
      SUB((pass+x));
      FLIP((pass+x));
    }
  printf("plaintext pass: \"");
  print(pass, len);
  printf("\"\n");


  return EXIT_SUCCESS;
}

