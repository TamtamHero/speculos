#ifndef _CX_AES_H
#define _CX_AES_H

#define CX_AES_BLOCK_SIZE 16
#define CX_LAST           (1<<0)
#define CX_MASK_SIGCRYPT  (3<<1)
#define CX_ENCRYPT        (2<<1)
#define CX_DECRYPT        (0<<1)
#define CX_PAD_NONE       (0<<3)
#define CX_MASK_CHAIN     (7<<6)
#define CX_CHAIN_ECB      (0<<6)
#define CX_CHAIN_CBC      (1<<6)

struct cx_aes_key_s {
  unsigned int     size;
  unsigned char keys[32];
};

typedef struct cx_aes_key_s cx_aes_key_t;

int sys_cx_aes_init_key(const uint8_t *raw_key,
                    unsigned int key_len,
                    cx_aes_key_t *key);
int sys_cx_aes(const cx_aes_key_t *key, int mode,
               const unsigned char *in, unsigned int len,
               unsigned char *out, unsigned int out_len);

#endif
