
#ifndef CRYPTO_H_
#define CRYPTO_H_

/* Default Crypto Calls*/
#define ENCRYPT_BUFF(in, len)			\
  ({int j;					\
  for (j=0; j<len; j++)				\
    ENCRYPT((in+j));				\
  }})
      
#define DECRYPT_BUFF(in, len)			\
  ({int j;					\
  for (j=0; j<len; j++)				\
    DECRYPT((in+j));				\
  }})

#define DECRYPT(in) (BASIC_DECRYPT(in))
#define ENCRYPT(in) (BASIC_ENCRYPT(in))

/* Basic Crypto */
#define BASIC_SUB_VAL (-7)
#define BASIC_XOR_VAL (0x7B)

#define BASIC_DECRYPT(in)	   \
  ({SWAP((in));			   \
  XOR_VAL((in), (BASIC_XOR_VAL));  \
  SUB_VAL((in), (BASIC_SUB_VAL));  \
  NOT((in)); })
  
#define BASIC_ENCRYPT(in)	   \
  ({NOT(in);			   \
  SUB_VAL((in), (BASIC_SUB_VAL));  \
  XOR_VAL((in), (BASIC_XOR_VAL));  \
  SWAP((in)); })



/* Crypto Operations */

#define SWAP_HALF(in) ((*in)=(((*in)&0x0F)<<4) | (((*in)&0xF0)>>4))

#define SWAP(in) ((*in)=(((*in)&0x0F)<<4) | (((*in)&0xF0)>>4))
//#define SWAP_INDX(in, indx)

#define XOR_VAL(in, xor) ((*in)=(*in)^(xor))
#define XOR_REF(in, xor) ((*in)=(*in)^(*xor))

#define MASK_VAL(in, mask) ((*in)=((*in)&(mask))))
#define MASK_REF(in, mask) ((*in)=((*in)&(*mask)))

#define SUB_VAL(in, sub) ((*in)-=(sub))
#define SUB_REF(in,sub) ((*in)-=(*sub))

#define NOT(in) ((*in)=~(*in))

#define MASK_HIGH(mask, size) \
  ({ mask = 0;		      \
    mask = ~mask;	      \
    mask = mask >> size;})

#define MASK_LOW(mask, size)  \
  ({ mask = 0;		      \
    mask = ~mask;	      \
    mask = mask << size;})

#endif

//list of macros to do crypto on internal values

/*CHANGE NAME OF FILE TO INTERN_CRYPTO - */
