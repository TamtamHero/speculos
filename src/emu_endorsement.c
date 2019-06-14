#include <string.h>

#include "cx.h"
#include "cx_ec.h"
#include "exception.h"
#include "emulate.h"

#define cx_ecdsa_init_public_key sys_cx_ecfp_init_public_key

// TODO: all keys are currently hardcoded

static cx_ecfp_private_key_t user_private_key_1 = {
  CX_CURVE_Ed25519,
  32,
  {
    0xad, 0xd4, 0xbb, 0x81, 0x03, 0x78, 0x5b, 0xaf, 0x9a, 0xc5, 0x34, 0x25,
    0x8e, 0x8a, 0xaf, 0x65, 0xf5, 0xf1, 0xad, 0xb5, 0xef, 0x5f, 0x3d, 0xf1,
    0x9b, 0xb8, 0x0a, 0xb9, 0x89, 0xc4, 0xd6, 0x4b
  },
};

static cx_ecfp_private_key_t user_private_key_2 = {
  CX_CURVE_Ed25519,
  32,
  {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a, 0xf4,
    0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
    0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
  },
};

#if 0
static uint8_t user_certificate_1_length;
static uint8_t user_certificate_1[1 +1 +2*(1+1+33)];

static uint8_t user_certificate_2_length;
static uint8_t user_certificate_2[1 +1 +2*(1+1+33)];
#endif


unsigned int sys_os_endorsement_get_code_hash(uint8_t *buffer) {
  memcpy(buffer, "12345678abcdef0000fedcba87654321", 32);
  return 32;
}

unsigned long sys_os_endorsement_get_public_key(uint8_t index, uint8_t *buffer) {
  cx_ecfp_private_key_t *privateKey;
  cx_ecfp_public_key_t publicKey;

  switch(index) {
    case 1:
      privateKey = &user_private_key_1;
      break;
    case 2:
      privateKey = &user_private_key_2;
      break;
    default:
      THROW(EXCEPTION);
      break;
  }

  cx_ecdsa_init_public_key(CX_CURVE_256K1, NULL, 0, &publicKey);

  sys_cx_ecfp_generate_pair(CX_CURVE_256K1, &publicKey, privateKey, 1);

  memcpy(buffer, publicKey.W, 65);

  return 65;
}

#if 0
unsigned int sys_os_endorsement_get_public_key_certificate(unsigned char index, unsigned char *buffer) {  
  unsigned char *certificate;
  unsigned char length;

  switch(index) {
    case 1:
      length = user_certificate_1_length;
      certificate = user_certificate_1;
      break;
    case 2:
      length = user_certificate_2_length;
      certificate = user_certificate_2;
      break;
    default:
      THROW(EXCEPTION);
      break;
  }

  if (length == 0) {
    THROW(EXCEPTION);
  }

  memcpy(buffer, certificate, length);

  return length;
}
#endif

unsigned long sys_os_endorsement_key1_sign_data(uint8_t *data, size_t dataLength, uint8_t *signature) {
  uint8_t hash[32];
  cx_sha256_t sha256;

  sys_os_endorsement_get_code_hash(hash);
  sys_cx_sha256_init(&sha256);
  sys_cx_hash((cx_hash_t *)&sha256, 0, data, dataLength, NULL, 0);
  sys_cx_hash((cx_hash_t *)&sha256, CX_LAST, hash, sizeof(hash), hash, 32);

  /* XXX: CX_RND_TRNG is set but actually ignored by speculos'
   *      sys_cx_ecdsa_sign implementation */
  sys_cx_ecdsa_sign(&user_private_key_1,
                    CX_LAST|CX_RND_TRNG, CX_SHA256,
                    hash,
                    sizeof(hash), // size of SHA256 hash
                    signature,
                    6+33*2, /*3TL+2V*/
                    NULL);

  return signature[1] + 2;
}
