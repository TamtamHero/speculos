#include <err.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/objects.h>

#include "cx_ec.h"
#include "cx_hash.h"
#include "exception.h"
#include "emulate.h"

static unsigned char const C_cx_secp256k1_a[]  = {
  // a:  0x00
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char const C_cx_secp256k1_b[]  = {
  //b:  0x07
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07};
static  unsigned char const C_cx_secp256k1_p []  = {
  //p:  0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2f};
static unsigned char const C_cx_secp256k1_Hp[]  = {
  //Hp: 0x000000000000000000000000000000000000000000000001000007a2000e90a1
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x07, 0xa2, 0x00, 0x0e, 0x90, 0xa1};
static unsigned char const C_cx_secp256k1_Gx[] = {
  //Gx: 0x79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
  0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
  0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98};
static unsigned char const C_cx_secp256k1_Gy[] = {
  //Gy:  0x483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
  0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8,
  0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19, 0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8};
static unsigned char const C_cx_secp256k1_n[]  = {
  //n: 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
  0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41};
static unsigned char const C_cx_secp256k1_Hn[]  = {
  //Hn:0x9d671cd581c69bc5e697f5e45bcd07c6741496c20e7cf878896cf21467d7d140
  0x9d, 0x67, 0x1c, 0xd5, 0x81, 0xc6, 0x9b, 0xc5, 0xe6, 0x97, 0xf5, 0xe4, 0x5b, 0xcd, 0x07, 0xc6,
  0x74, 0x14, 0x96, 0xc2, 0x0e, 0x7c, 0xf8, 0x78, 0x89, 0x6c, 0xf2, 0x14, 0x67, 0xd7, 0xd1, 0x40};

#define C_cx_secp256k1_h  1

cx_curve_weierstrass_t const C_cx_secp256k1 = {
  CX_CURVE_SECP256K1,
  256, 32,
  (unsigned char*)C_cx_secp256k1_p,
  (unsigned char*)C_cx_secp256k1_Hp,
  (unsigned char*)C_cx_secp256k1_Gx,
  (unsigned char*)C_cx_secp256k1_Gy,
  (unsigned char*)C_cx_secp256k1_n,
  (unsigned char*)C_cx_secp256k1_Hn,
  C_cx_secp256k1_h,
  (unsigned char*)C_cx_secp256k1_a,
  (unsigned char*)C_cx_secp256k1_b,
};

cx_curve_domain_t const * const C_cx_allCurves[]= {
  (const cx_curve_domain_t *)&C_cx_secp256k1,
};

static int nid_from_curve(cx_curve_t curve) {
  int nid;

  switch (curve) {
  case CX_CURVE_SECP256K1:
    nid = NID_secp256k1;
    break;
#if 0
  case CX_CURVE_SECP256R1:
    nid = NID_X9_62_prime256v1;
    break;
  case CX_CURVE_SECP384R1:
    nid = NID_secp384r1;
    break;
  case CX_CURVE_SECP521R1:
    nid = NID_secp521r1;
    break;
  case CX_CURVE_BrainPoolP256R1:
    nid = NID_brainpoolP256r1;
    break;
  case CX_CURVE_BrainPoolP320R1:
    nid = NID_brainpoolP320r1;
    break;
  case CX_CURVE_BrainPoolP384R1:
    nid = NID_brainpoolP384r1;
    break;
  case CX_CURVE_BrainPoolP512R1:
    nid = NID_brainpoolP512r1;
    break;
#endif
  default:
    nid = -1;
    errx(1, "cx_ecfp_generate_pair unsupported curve");
    break;
  }
  return nid;
}

int sys_cx_ecfp_generate_pair(cx_curve_t curve, cx_ecfp_public_key_t *public_key, cx_ecfp_private_key_t *private_key, int keep_private)
{
  const EC_GROUP *group;
  EC_POINT *pub;
  BN_CTX *ctx;
  EC_KEY *key;
  BIGNUM *bn;
  int nid;

  nid = nid_from_curve(curve);
  key = EC_KEY_new_by_curve_name(nid);
  if (key == NULL) {
    errx(1, "ssl: EC_KEY_new_by_curve_name");
  }

  group = EC_KEY_get0_group(key);

  ctx = BN_CTX_new();

  if (!keep_private) {
    if (EC_KEY_generate_key(key) == 0) {
      errx(1, "ssl: EC_KEY_generate_key");
    }

    bn = (BIGNUM *)EC_KEY_get0_private_key(key);
    if (BN_num_bytes(bn) > (int)sizeof(private_key->d)) {
      errx(1, "ssl: invalid bn");
    }
    private_key->curve = curve;
    private_key->d_len = BN_bn2bin(bn, private_key->d);
  } else {
    BIGNUM *priv;

    priv = BN_new();
    BN_bin2bn(private_key->d, private_key->d_len, priv);
    if (EC_KEY_set_private_key(key, priv) == 0) {
      errx(1, "ssl: EC_KEY_set_private_key");
    }

    pub = EC_POINT_new(group);
    if (EC_POINT_mul(group, pub, priv, NULL, NULL, ctx) == 0) {
      errx(1, "ssl: EC_POINT_mul");
    }

    if (EC_KEY_set_public_key(key, pub) == 0) {
      errx(1, "ssl: EC_KEY_set_public_key");
    }

    BN_free(priv);
  }

  bn = BN_new();
  pub = (EC_POINT *)EC_KEY_get0_public_key(key);
  EC_POINT_point2bn(group, pub, POINT_CONVERSION_UNCOMPRESSED, bn, ctx);
  if (BN_num_bytes(bn) > (int)sizeof(public_key->W)) {
    errx(1, "ssl: invalid bn");
  }
  public_key->curve = curve;
  public_key->W_len = BN_bn2bin(bn, public_key->W);

  EC_KEY_free(key);
  BN_CTX_free(ctx);

  return 0;
}

int sys_cx_ecfp_init_private_key(cx_curve_t curve, const uint8_t *raw_key, unsigned int key_len, cx_ecfp_private_key_t *key)
{
  /* TODO: missing checks */

  if (raw_key != NULL) {
    key->d_len = key_len;
    memmove(key->d, raw_key, key_len);
  } else {
    key_len = 0;
  }

  key->curve = curve;

  return key_len;
}

static int asn1_read_len(uint8_t **p, const uint8_t *end, size_t *len) {
  /* Adapted from secp256k1 */
  int lenleft;
  unsigned int b1;
  *len = 0;

  if (*p >= end)
    return 0;

  b1 = *((*p)++);
  if (b1 == 0xff) {
    /* X.690-0207 8.1.3.5.c the value 0xFF shall not be used. */
    return 0;
  }
  if ((b1 & 0x80u) == 0) {
    /* X.690-0207 8.1.3.4 short form length octets */
    *len = b1;
    return 1;
  }
  if (b1 == 0x80) {
    /* Indefinite length is not allowed in DER. */
    return 0;
  }
  /* X.690-207 8.1.3.5 long form length octets */
  lenleft = b1 & 0x7Fu;
  if (lenleft > end - *p) {
    return 0;
  }
  if (**p == 0) {
    /* Not the shortest possible length encoding. */
    return 0;
  }
  if ((size_t)lenleft > sizeof(size_t)) {
    /* The resulting length would exceed the range of a size_t, so
     * certainly longer than the passed array size.
     */
    return 0;
  }
  while (lenleft > 0) {
    if ((*len >> ((sizeof(size_t) - 1) * 8)) != 0) {
    }
    *len = (*len << 8u) | **p;
    if (*len + lenleft > (size_t)(end - *p)) {
      /* Result exceeds the length of the passed array. */
      return 0;
    }
    (*p)++;
    lenleft--;
  }
  if (*len < 128) {
    /* Not the shortest possible length encoding. */
    return 0;
  }
  return 1;
}

static int asn1_read_tag(uint8_t **p, const uint8_t *end, size_t *len, int tag) {
  if ((end - *p) < 1) return 0;

  if (**p != tag) return 0;

  (*p)++;
  return asn1_read_len(p, end, len);
}

static int asn1_parse_integer(uint8_t **p, const uint8_t *end, uint8_t **n, size_t *n_len) {
  size_t len;
  int ret = 0;

  if (!asn1_read_tag(p, end, &len, 0x02)) /* INTEGER */
    goto end;

  if (((*p)[0] & 0x80u) == 0x80u) {
    /* Truncated, missing leading 0 (negative number) */
    goto end;
  }

  if ((*p)[0] == 0 && len >= 2 && ((*p)[1] & 0x80u) == 0) {
    /* Zeroes have been prepended to the integer */
    goto end;
  }

  while (**p == 0 && *p != end && len > 0) { /* Skip leading null bytes */
    (*p)++;
    len--;
  }

  *n = *p;
  *n_len = len;

  *p += len;
  ret = 1;

  end:
  return ret;
}

int cx_ecfp_decode_sig_der(const uint8_t *input, size_t input_len,
    size_t max_size, uint8_t **r, size_t *r_len, uint8_t **s, size_t *s_len) {
  size_t len;
  int ret = 0;
  const uint8_t *input_end = input + input_len;

  uint8_t *p = (uint8_t *)input;

  if (!asn1_read_tag(&p, input_end, &len, 0x30)) /* SEQUENCE */
    goto end;

  if (p + len != input_end) goto end;

  if (!asn1_parse_integer(&p, input_end, r, r_len) ||
      !asn1_parse_integer(&p, input_end, s, s_len))
    goto end;

  if (p != input_end) /* Check if bytes have been appended to the sequence */
    goto end;

  if (*r_len > max_size || *s_len > max_size) {
    return 0;
  }
  ret = 1;
  end:
  return ret;
}

const cx_curve_domain_t *cx_ecfp_get_domain(cx_curve_t curve) {
  unsigned int i;
  for (i = 0; i<sizeof(C_cx_allCurves)/sizeof(cx_curve_domain_t*); i++) {
    if (C_cx_allCurves[i]->curve == curve) {
      return C_cx_allCurves[i];
    }
  }
  THROW(INVALID_PARAMETER);
}

int sys_cx_ecdsa_verify(const cx_ecfp_public_key_t *key, int UNUSED(mode), cx_md_t UNUSED(hashID), const uint8_t *hash, unsigned int hash_len, const uint8_t *sig,  unsigned int sig_len)
{
  cx_curve_weierstrass_t      *domain;
  unsigned int                 size;
  unsigned char               *r, *s;
  size_t rlen, slen;
  int nid = 0;

  domain = (cx_curve_weierstrass_t *)cx_ecfp_get_domain(key->curve);
  size = domain->length; //bits  -> bytes

  if (!cx_ecfp_decode_sig_der(sig, sig_len, size, &r, &rlen, &s, &slen)) {
    return 0;
  }

  // Load ECDSA signature
  BIGNUM *sig_r = BN_new();
  BIGNUM *sig_s = BN_new();
  BN_bin2bn(r, rlen, sig_r);
  BN_bin2bn(s, slen, sig_s);
  ECDSA_SIG *ecdsa_sig = ECDSA_SIG_new();
  ECDSA_SIG_set0(ecdsa_sig, sig_r, sig_s);

  // Set public key
  BIGNUM *x = BN_new();
  BIGNUM *y = BN_new();
  BN_bin2bn(key->W + 1, domain->length, x);
  BN_bin2bn(key->W + domain->length + 1, domain->length, y);

  switch (key->curve) {
  case CX_CURVE_SECP256K1:
    nid = NID_secp256k1;
    break;
  case CX_CURVE_SECP256R1:
    nid = NID_X9_62_prime256v1;
    break;
  case CX_CURVE_SECP384R1:
    nid = NID_secp384r1;
    break;
  case CX_CURVE_SECP521R1:
    nid = NID_secp521r1;
    break;
  case CX_CURVE_BrainPoolP256R1:
    nid = NID_brainpoolP256r1;
    break;
  case CX_CURVE_BrainPoolP320R1:
    nid = NID_brainpoolP320r1;
    break;
  case CX_CURVE_BrainPoolP384R1:
    nid = NID_brainpoolP384r1;
    break;
  case CX_CURVE_BrainPoolP512R1:
    nid = NID_brainpoolP512r1;
    break;
  default:
    break;
  }

  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_public_key_affine_coordinates(ec_key, x, y);

  int ret = ECDSA_do_verify(hash, hash_len, ecdsa_sig, ec_key);
  if (ret != 1) {
    ret = 0;
  }

  ECDSA_SIG_free(ecdsa_sig);

  BN_free(y);
  BN_free(x);
  EC_KEY_free(ec_key);
  return ret;
}

int cx_ecfp_encode_sig_der(unsigned char* sig, unsigned int sig_len,
                           unsigned char* r, unsigned int r_len, unsigned char* s, unsigned int s_len) {
  unsigned int offset;

  while ((*r == 0) && r_len) {
    r++;
    r_len--;
  }
  while ((*s == 0) && s_len) {
    s++;
    s_len--;
  }
  if (!r_len || !s_len) {
    return 0;
  }

  //check sig_len
  offset = 3*2+r_len+s_len;
  if (*r&0x80) offset++;
  if (*s&0x80) offset++;
  if (sig_len < offset) {
    return 0;
  }

  //r
  offset = 2;
  if (*r&0x80) {
    sig[offset+2] = 0;
    memmove(sig+offset+3, r, r_len);
    r_len++;
  } else {
    memmove(sig+offset+2, r, r_len);
  }
  sig[offset] = 0x02;
  sig[offset+1] = r_len;

  //s
  offset += 2+r_len;
  if (*s&0x80) {
    sig[offset+2] = 0;
    memmove(sig+offset+3, s, s_len);
    s_len++;
  } else {
    memmove(sig+offset+2, s, s_len);
  }
  sig[offset] = 0x02;
  sig[offset+1] = s_len;

  //head
  sig[0] = 0x30;
  sig[1] = 2+r_len+2+s_len;

  return 2+sig[1];
}

int sys_cx_ecdsa_sign(const cx_ecfp_private_key_t *key, int UNUSED(mode), cx_md_t UNUSED(hashID), const uint8_t *hash, unsigned int hash_len, uint8_t *sig, unsigned int sig_len, unsigned int *UNUSED(info))
{
  int nid = 0;
  uint8_t *buf_r, *buf_s;

  const cx_curve_domain_t *domain = cx_ecfp_get_domain(key->curve);
  nid = nid_from_curve(key->curve);
  if (nid < 0) {
    return 0;
  }

  const BIGNUM *r, *s;
  BIGNUM *x = BN_new();
  BN_bin2bn(key->d, key->d_len, x);
  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);

  EC_KEY_set_private_key(ec_key, x);
  ECDSA_SIG *ecdsa_sig = ECDSA_do_sign(hash, hash_len, ec_key);

  buf_r = malloc(domain->length);
  buf_s = malloc(domain->length);

  ECDSA_SIG_get0(ecdsa_sig, &r, &s);
  BN_bn2binpad(r, buf_r, domain->length);
  BN_bn2binpad(s, buf_s, domain->length);
  int ret = cx_ecfp_encode_sig_der(sig, sig_len, buf_r, domain->length, buf_s,
                                   domain->length);

  free(buf_r);
  free(buf_s);
  ECDSA_SIG_free(ecdsa_sig);
  EC_KEY_free(ec_key);
  BN_free(x);
  return ret;
}
