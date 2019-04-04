#include <err.h>
#include <stdio.h>

#include "cx.h"
#include "cx_hash.h"
#include "cx_math.h"
#include "emulate.h"
#include "bolos_syscalls.h"

#define SYSCALL0(_name)                              \
  case SYSCALL_ ## _name ## _ID_IN: {                \
    *ret = sys_ ## _name();                          \
    retid = SYSCALL_ ## _name ## _ID_OUT;            \
    break;                                           \
  }

#define SYSCALL1(_name, _type0, _arg0)               \
  case SYSCALL_ ## _name ## _ID_IN: {                \
    _type0 _arg0 = (_type0)parameters[0];            \
    *ret = sys_ ## _name(_arg0);                     \
    retid = SYSCALL_ ## _name ## _ID_OUT;            \
    break;                                           \
  }

#define SYSCALL2(_name, _type0, _arg0, _type1, _arg1)                 \
  case SYSCALL_ ## _name ## _ID_IN: {                                 \
    _type0 _arg0 = (_type0)parameters[0];                             \
    _type1 _arg1 = (_type1)parameters[1];                             \
    *ret = sys_ ## _name(_arg0, _arg1);                               \
    retid = SYSCALL_ ## _name ## _ID_OUT;                             \
    break;                                                            \
  }

#define SYSCALL3(_name, _type0, _arg0, _type1, _arg1, _type2, _arg2)  \
  case SYSCALL_ ## _name ## _ID_IN: {                                 \
    _type0 _arg0 = (_type0)parameters[0];                             \
    _type1 _arg1 = (_type1)parameters[1];                             \
    _type2 _arg2 = (_type2)parameters[2];                             \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2);                        \
    retid = SYSCALL_ ## _name ## _ID_OUT;                             \
    break;                                                            \
  }

#define SYSCALL4(_name, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3);                   \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL5(_name, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4);            \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL6(_name, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5);     \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL7(_name, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    _type6 _arg6 = (_type6)parameters[6];                               \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6); \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL8(_name, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6, _type7, _arg7) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    _type6 _arg6 = (_type6)parameters[6];                               \
    _type7 _arg7 = (_type7)parameters[7];                               \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7); \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

int emulate(unsigned long syscall, unsigned long *parameters, unsigned long *ret)
{
  int retid;

  switch(syscall) {
  case SYSCALL_check_api_level_ID_IN:
    retid = SYSCALL_check_api_level_ID_OUT;
    break;

  SYSCALL6(cx_blake2b_init2,
           cx_blake2b_t *, hash,
           unsigned int,   size,
           uint8_t *,      salt,
           unsigned int,   salt_len,
           uint8_t *,      perso,
           unsigned int,   perso_len);

  SYSCALL8(cx_ecdsa_sign,
           const cx_ecfp_private_key_t *, key,
           int,                           mode,
           cx_md_t,                       hashID,
           const uint8_t *,               hash,
           unsigned int,                  hash_len,
           uint8_t *,                     sig,
           unsigned int,                  sig_len,
           unsigned int *,                info);

  SYSCALL7(cx_ecdsa_verify,
           const cx_ecfp_public_key_t *, key,
           int,                          mode,
           cx_md_t,                      hashID,
           const uint8_t *,              hash,
           unsigned int,                 hash_len,
           const uint8_t *,              sig,
           unsigned int,                 sig_len);

  SYSCALL4(cx_ecfp_generate_pair,
           cx_curve_t,              curve,
           cx_ecfp_public_key_t *,  public_key,
           cx_ecfp_private_key_t *, private_key,
           int,                     keep_private);

  SYSCALL4(cx_ecfp_init_private_key,
           cx_curve_t,              curve,
           const uint8_t *,         raw_key,
           unsigned int,            key_len,
           cx_ecfp_private_key_t *, key);

  SYSCALL6(cx_hash,
           cx_hash_t *,     hash,
           int,             mode,
           const uint8_t *, in,
           size_t,          len,
           uint8_t *,       out,
           size_t,          out_len);

  SYSCALL6(cx_hmac_sha256,
           const uint8_t *, key,
           unsigned int,    key_len,
           const uint8_t *, in,
           unsigned int,    len,
           uint8_t *,       out,
           unsigned int,    out_len);

  SYSCALL5(cx_math_addm,
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           const uint8_t *, m,
           unsigned int,    len);

  SYSCALL3(cx_math_cmp,
           const uint8_t *, a,
           const uint8_t *, b,
           unsigned int,    len);

  SYSCALL2(cx_math_is_prime, const uint8_t *, r, unsigned int, len);

  SYSCALL2(cx_math_is_zero, const uint8_t *, a, unsigned int, len);

  SYSCALL4(cx_math_modm,
           uint8_t *,       v,
           unsigned int,    len_v,
           const uint8_t *, m,
           unsigned int,    len_m);

  SYSCALL5(cx_math_multm,
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           const uint8_t *, m,
           unsigned int,    len);

  SYSCALL1(cx_ripemd160_init, cx_ripemd160_t *, hash);

  SYSCALL2(cx_rng, uint8_t *, buffer, unsigned int, length);

  SYSCALL1(cx_sha256_init, cx_sha256_t *, hash);

  SYSCALL0(io_seproxyhal_spi_is_status_sent);

  SYSCALL3(io_seproxyhal_spi_recv, uint8_t *, buffer, uint16_t, maxlength, unsigned int, flags);

  SYSCALL2(io_seproxyhal_spi_send, uint8_t *, buffer, uint16_t, length);

  SYSCALL3(nvm_write, void *, dst_addr, void *, src_addr, size_t, src_len);

  SYSCALL0(os_flags);

  SYSCALL0(os_global_pin_is_validated);

  SYSCALL1(os_lib_call, unsigned long *, call_parameters);

  case SYSCALL_os_lib_throw_ID_IN:
    retid = SYSCALL_os_lib_throw_ID_OUT;
    break;

  SYSCALL5(os_perso_derive_node_bip32,
           cx_curve_t,       curve,
           const uint32_t *, path,
           size_t,           length,
           uint8_t*,         private_key,
           uint8_t *,        chain
           );

  SYSCALL3(os_registry_get_current_app_tag,
           unsigned int, tag,
           uint8_t *,    buffer,
           size_t,       length);

  case SYSCALL_os_sched_exit_ID_IN:
    fprintf(stderr, "[*] exit syscall called (skipped)\n");
    retid = SYSCALL_os_sched_exit_ID_OUT;
    break;

  SYSCALL1(os_ux, bolos_ux_params_t *, params);

  case SYSCALL_reset_ID_IN:
    retid = SYSCALL_reset_ID_OUT;
    break;

  default:
    errx(1, "failed to emulate syscall 0x%08lx", syscall);
    break;
  }

  return retid;
}
