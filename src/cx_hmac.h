#ifndef _CX_HMAC_H
#define _CX_HMAC_H

int cx_hmac_sha512_init(cx_hmac_sha512_t *hmac, const unsigned char *key, unsigned int key_len);
int sys_cx_hmac_sha256_init(cx_hmac_sha256_t *hmac, const unsigned char *key, unsigned int key_len);

#endif
