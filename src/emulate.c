#include <err.h>
#include <stdio.h>

#include "cx.h"
#include "cx_aes.h"
#include "cx_ec.h"
#include "cx_crc.h"
#include "cx_hash.h"
#include "cx_hmac.h"
#include "cx_math.h"
#include "emulate.h"
#include "bolos_syscalls.h"

#define print_syscall(fmt, ...)                           \
  do {                                                    \
    if (verbose) {                                        \
      fprintf(stderr, "[*] syscall: " fmt, __VA_ARGS__);  \
    }                                                     \
  } while (0)

#define print_ret(ret)                                    \
  do {                                                    \
    if (verbose) {                                        \
      fprintf(stderr, " = %ld\n", ret);                   \
    }                                                     \
  } while (0)

#define SYSCALL0(_name)                              \
  case SYSCALL_ ## _name ## _ID_IN: {                \
    *ret = sys_ ## _name();                          \
    print_syscall(# _name "(%s)", "");               \
    retid = SYSCALL_ ## _name ## _ID_OUT;            \
    print_ret(*ret);                                 \
    break;                                           \
  }

#define SYSCALL1(_name, _fmt, _type0, _arg0)          \
  case SYSCALL_ ## _name ## _ID_IN: {                 \
    _type0 _arg0 = (_type0)parameters[0];             \
    print_syscall(# _name "" _fmt, (_type0)_arg0);    \
    *ret = sys_ ## _name(_arg0);                      \
    print_ret(*ret);                                  \
    retid = SYSCALL_ ## _name ## _ID_OUT;             \
    break;                                            \
  }

#define SYSCALL2(_name, _fmt, _type0, _arg0, _type1, _arg1)             \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1);       \
    *ret = sys_ ## _name(_arg0, _arg1);                                 \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL3(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2);                          \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL4(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3);                   \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL5(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4);            \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL6(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4, (_type5)_arg5); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5);     \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL7(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    _type6 _arg6 = (_type6)parameters[6];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6); \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL8(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6, _type7, _arg7) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    _type6 _arg6 = (_type6)parameters[6];                               \
    _type7 _arg7 = (_type7)parameters[7];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6, (_type7)_arg7); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7); \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

int emulate(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose)
{
  int retid;

  switch(syscall) {
  SYSCALL0(check_api_level);

  SYSCALL6(cx_aes, "(%p, 0x%x, %p, %u, %p, %u)",
           const cx_aes_key_t *, key,
           int,                  mode,
           const uint8_t *,      in,
           unsigned int,         len,
           uint8_t *,            out,
           unsigned int,         out_len);

  SYSCALL3(cx_aes_init_key, "(%p, %u, %p)",
           const uint8_t *, raw_key,
           unsigned int,    key_len,
           cx_aes_key_t *,  key);

  SYSCALL6(cx_blake2b_init2, "(%p, %u, %p, %u, %p, %u)",
           cx_blake2b_t *, hash,
           unsigned int,   size,
           uint8_t *,      salt,
           unsigned int,   salt_len,
           uint8_t *,      perso,
           unsigned int,   perso_len);

  SYSCALL3(cx_crc16_update, "(%u, %p, %u)",
           unsigned short, crc, const void *, b, size_t, len);

  SYSCALL6(cx_ecdh, "(%p, %d, %p, %u, %p, %u)",
          const cx_ecfp_private_key_t *, key,
          int,                           mode,
          const uint8_t *,               public_point,
          size_t,                        P_len,
          uint8_t *,                     secret,
          size_t,                        secret_len);

  SYSCALL8(cx_ecdsa_sign, "(%p, 0x%x, %d, %p, %u, %p, %u, %p)",
           const cx_ecfp_private_key_t *, key,
           int,                           mode,
           cx_md_t,                       hashID,
           const uint8_t *,               hash,
           unsigned int,                  hash_len,
           uint8_t *,                     sig,
           unsigned int,                  sig_len,
           unsigned int *,                info);

  SYSCALL7(cx_ecdsa_verify, "(%p, 0x%x, %d, %p, %u, %p, %u)",
           const cx_ecfp_public_key_t *, key,
           int,                          mode,
           cx_md_t,                      hashID,
           const uint8_t *,              hash,
           unsigned int,                 hash_len,
           const uint8_t *,              sig,
           unsigned int,                 sig_len);

  SYSCALL4(cx_ecfp_generate_pair, "(0x%x, %p, %p, %d)",
           cx_curve_t,              curve,
           cx_ecfp_public_key_t *,  public_key,
           cx_ecfp_private_key_t *, private_key,
           int,                     keep_private);

  SYSCALL4(cx_ecfp_init_private_key, "(0x%x, %p, %u, %p)",
           cx_curve_t,              curve,
           const uint8_t *,         raw_key,
           unsigned int,            key_len,
           cx_ecfp_private_key_t *, key);

  SYSCALL6(cx_hash, "(%p, 0x%x, %p, %u, %p, %u)",
           cx_hash_t *,     hash,
           int,             mode,
           const uint8_t *, in,
           size_t,          len,
           uint8_t *,       out,
           size_t,          out_len);

  SYSCALL4(cx_hash_sha256, "(%p, %u, %p, %u)",
           const uint8_t *, in,
           size_t,          len,
           uint8_t *,       out,
           size_t,          out_len);

  SYSCALL6(cx_hmac, "(%p, 0x%x, %p, %u, %p, %u)",
           cx_hmac_t *,     hmac,
           int,             mode,
           const uint8_t *, in,
           unsigned int,    len,
           uint8_t *,       out,
           unsigned int,    out_len);

  SYSCALL6(cx_hmac_sha256, "(%p, %u, %p, %u, %p, %u)",
           const uint8_t *, key,
           unsigned int,    key_len,
           const uint8_t *, in,
           unsigned int,    len,
           uint8_t *,       out,
           unsigned int,    out_len);

  SYSCALL3(cx_hmac_sha256_init, "(%p, %p, %u)",
           cx_hmac_sha256_t *, hmac,
           const uint8_t *,    key,
           unsigned int,       key_len);

  SYSCALL5(cx_math_addm, "(%p, %p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           const uint8_t *, m,
           unsigned int,    len);

  SYSCALL3(cx_math_cmp, "(%p, %p, %u)",
           const uint8_t *, a,
           const uint8_t *, b,
           unsigned int,    len);

  SYSCALL2(cx_math_is_prime, "(%p, %u)",
           const uint8_t *, r,
           unsigned int,    len);

  SYSCALL2(cx_math_is_zero, "(%p, %u)",
           const uint8_t *, a,
           unsigned int,    len);

  SYSCALL4(cx_math_modm, "(%p, %u, %p, %u)",
           uint8_t *,       v,
           unsigned int,    len_v,
           const uint8_t *, m,
           unsigned int,    len_m);

  SYSCALL5(cx_math_multm, "(%p, %p, %p, %p, %u)",
           uint8_t *,       r,
           const uint8_t *, a,
           const uint8_t *, b,
           const uint8_t *, m,
           unsigned int,    len);

  SYSCALL1(cx_ripemd160_init, "(%p)", cx_ripemd160_t *, hash);

  SYSCALL2(cx_rng, "(%p, %u)",
           uint8_t *,    buffer,
           unsigned int, length);

  SYSCALL1(cx_sha256_init, "(%p)", cx_sha256_t *, hash);

  SYSCALL0(io_seproxyhal_spi_is_status_sent);
  SYSCALL0(io_seph_is_status_sent);

  SYSCALL3(io_seproxyhal_spi_recv, "(%p, %u, 0x%x)",
           uint8_t *,    buffer,
           uint16_t,     maxlength,
           unsigned int, flags);
  SYSCALL3(io_seph_recv, "(%p, %u, 0x%x)",
           uint8_t *,    buffer,
           uint16_t,     maxlength,
           unsigned int, flags);

  SYSCALL2(io_seproxyhal_spi_send, "(%p, %u)",
           uint8_t *, buffer,
           uint16_t,  length);
  SYSCALL2(io_seph_send, "(%p, %u)",
           uint8_t *, buffer,
           uint16_t,  length);

  SYSCALL3(nvm_write,  "(%p, %p, %u)",
           void *, dst_addr,
           void *, src_addr,
           size_t, src_len);

  SYSCALL0(os_flags);

  SYSCALL0(os_global_pin_is_validated);

  SYSCALL0(os_global_pin_invalidate);

  SYSCALL1(os_lib_call, "(%p)", unsigned long *, call_parameters);

  SYSCALL1(os_lib_throw, "(0x%x)",
           unsigned int, exception);

  SYSCALL0(os_perso_isonboarded);

  SYSCALL5(os_perso_derive_node_bip32, "(0x%x, %p, %u, %p, %p)",
           cx_curve_t,       curve,
           const uint32_t *, path,
           size_t,           length,
           uint8_t*,         private_key,
           uint8_t *,        chain
           );

  SYSCALL3(os_registry_get_current_app_tag, "(0x%x, %p, %u)",
           unsigned int, tag,
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL0(os_sched_exit);

  SYSCALL0(os_sched_last_status);

  SYSCALL1(os_ux, "(%p)", bolos_ux_params_t *, params);

  SYSCALL0(try_context_get);

  SYSCALL1(try_context_set, "(%p)",
           try_context_t *, context);

  SYSCALL0(reset);

  default:
    errx(1, "failed to emulate syscall 0x%08lx", syscall);
    break;
  }

  return retid;
}
