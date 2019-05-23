#include <err.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/aes.h>

#include "cx.h"
#include "cx_aes.h"
#include "exception.h"

static void cx_aes_enc_block(const cx_aes_key_t *key, const unsigned char *inblock, unsigned char *outblock) {
  AES_KEY aes_key;

  AES_set_encrypt_key(key->keys, key->size * 8, &aes_key);
  AES_encrypt(inblock, outblock, &aes_key);
  OPENSSL_cleanse(&aes_key, sizeof(aes_key));
}

static void cx_aes_dec_block(const cx_aes_key_t *key, const unsigned char *inblock, unsigned char *outblock) {
  AES_KEY aes_key;

  AES_set_decrypt_key(key->keys, key->size * 8, &aes_key);
  AES_decrypt(inblock, outblock, &aes_key);
  OPENSSL_cleanse(&aes_key, sizeof(aes_key));
}

int sys_cx_aes_init_key(const unsigned char *raw_key,
                        unsigned int key_len,
                        cx_aes_key_t *key) {
  memset(key, 0, sizeof(cx_aes_key_t));
  switch(key_len) {
  case 16:
  case 24:
  case 32:
    key->size = key_len;
    memmove(key->keys, raw_key, key_len);
    return key_len;
  default:
    THROW(INVALID_PARAMETER);
    return -1;
  }
}

static void cx_memxor(uint8_t *buf1, const uint8_t *buf2, size_t len) {
  size_t i;

  for (i = 0; i < len; i++) {
    buf1[i] ^= buf2[i];
  }
}

int sys_cx_aes(const cx_aes_key_t *key, int mode,
               const unsigned char *in, unsigned int len,
               unsigned char *out, unsigned int out_len) {
  unsigned char block[CX_AES_BLOCK_SIZE], last[CX_AES_BLOCK_SIZE*2];
  unsigned int len_out;

  if (len % CX_AES_BLOCK_SIZE != 0) {
    err(1, "cx_aes: unsupported size %u (the vault app is the only app currently supported), please open an issue", len);
  }

  if (!(mode & (CX_LAST | CX_PAD_NONE))) {
    err(1, "cx_aes: unsupported mode (the vault app is the only app currently supported), please open an issue");
  }

  mode &= ~(CX_LAST | CX_PAD_NONE);
  if (mode & ~(CX_ENCRYPT | CX_DECRYPT | CX_CHAIN_CBC | CX_CHAIN_ECB)) {
    err(1, "cx_aes: unsupported mode (the vault app is the only app currently supported), please open an issue");
  }

  memset(block, 0, sizeof(block));
  memset(last, 0, sizeof(last));

  if ((mode & CX_MASK_CHAIN) == CX_CHAIN_CBC) {
    if ((mode & CX_MASK_SIGCRYPT) == CX_ENCRYPT) {
      len_out = len - len %  CX_AES_BLOCK_SIZE;
      while (len >= CX_AES_BLOCK_SIZE) {
        cx_memxor(block, in,  CX_AES_BLOCK_SIZE);
        cx_aes_enc_block(key, block, block);
        memmove(out, block, CX_AES_BLOCK_SIZE);
        len -= CX_AES_BLOCK_SIZE;
        in += CX_AES_BLOCK_SIZE;
        out_len -= CX_AES_BLOCK_SIZE;
        out += CX_AES_BLOCK_SIZE;
      }
    }
    else if ((mode & CX_MASK_SIGCRYPT) == CX_DECRYPT) {
      len_out = len - CX_AES_BLOCK_SIZE;
      while (len > CX_AES_BLOCK_SIZE) {
        memmove(block, in, CX_AES_BLOCK_SIZE);
        cx_aes_dec_block(key, block, block);
        cx_memxor(block, last, CX_AES_BLOCK_SIZE);
        memmove(last, in, CX_AES_BLOCK_SIZE);
        memmove(out, block, CX_AES_BLOCK_SIZE);
        len -= CX_AES_BLOCK_SIZE;
        in += CX_AES_BLOCK_SIZE;
        out_len -= CX_AES_BLOCK_SIZE;
        out += CX_AES_BLOCK_SIZE;
      }

      memmove(block, in, CX_AES_BLOCK_SIZE);
      cx_aes_dec_block(key, block, block);
      cx_memxor(block, last, CX_AES_BLOCK_SIZE);
      memmove(out, block, CX_AES_BLOCK_SIZE);
      len_out += CX_AES_BLOCK_SIZE;
    }
    else {
      err(1, "cx_aes: unsupported mode (the vault app is the only app currently supported), please open an issue");
    }
  }
  else if ((mode & CX_MASK_CHAIN) ==  CX_CHAIN_ECB) {
    if ((mode & CX_MASK_SIGCRYPT) == CX_ENCRYPT) {
      len_out = len - len % CX_AES_BLOCK_SIZE;
      while (len >= CX_AES_BLOCK_SIZE) {
        cx_aes_enc_block(key, in, out);
        len -= CX_AES_BLOCK_SIZE;
        in += CX_AES_BLOCK_SIZE;
        out += CX_AES_BLOCK_SIZE;
      }
    }
    else {
      err(1, "cx_aes: unsupported mode (the vault app is the only app currently supported), please open an issue");
    }
  }
  else {
    err(1, "acx_aes: unsupported mode (the vault app is the only app currently supported), please open an issue");
  }

  return len_out;
}
