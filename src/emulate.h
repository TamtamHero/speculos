#ifndef _EMULATE_H
#define _EMULATE_H

/* for ucontext_t */
#include <signal.h>

/* for bool */
#include <stdbool.h>

/* for uint8_t, uint16_t, etc. */
#include <stdint.h>

/* for cx_curve_t */
#include "cx_ec.h"

/* for cx_hash_t */
#include "cx_hash.h"

/* TODO */
typedef void bolos_ux_params_t;
typedef struct try_context_s try_context_t;

typedef struct cx_ecfp_256_public_key_s cx_ecfp_public_key_t;
typedef struct cx_ecfp_256_private_key_s cx_ecfp_private_key_t;

#define SEPH_FILENO	0 /* 0 is stdin fileno */

#ifndef UNUSED
# ifdef __GNUC__
#   define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
# else
#   define UNUSED(x) UNUSED_ ## x
# endif
#endif

int emulate(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose);

unsigned long sys_os_lib_call(unsigned long *parameters);
unsigned long sys_os_global_pin_is_validated(void);
unsigned long sys_os_global_pin_invalidate(void);
unsigned long sys_os_flags(void);
int sys_nvm_write(void *dst_addr, void* src_addr, size_t src_len);
unsigned long sys_os_perso_derive_node_bip32(cx_curve_t curve, const uint32_t *path, size_t length, uint8_t *private_key, uint8_t* chain);
unsigned long sys_os_registry_get_current_app_tag(unsigned int tag, uint8_t *buffer, size_t length);
unsigned long sys_os_ux(bolos_ux_params_t *params);

int sys_cx_ecdsa_sign(const cx_ecfp_private_key_t *key, int mode, cx_md_t hashID, const uint8_t *hash, unsigned int hash_len, uint8_t *sig, unsigned int sig_len, unsigned int *info);
int sys_cx_ecdsa_verify(const cx_ecfp_public_key_t *key, int mode, cx_md_t hashID, const uint8_t *hash, unsigned int hash_len, const uint8_t *sig,  unsigned int sig_len);
int sys_cx_ecfp_generate_pair(cx_curve_t curve, cx_ecfp_public_key_t *public_key, cx_ecfp_private_key_t *private_key, int keep_private);
int sys_cx_ecfp_init_private_key(cx_curve_t curve, const uint8_t *raw_key, unsigned int key_len, cx_ecfp_private_key_t *key);
unsigned long sys_cx_hash(cx_hash_t *hash, int mode, const uint8_t *in, size_t len, uint8_t *out, size_t out_len);
unsigned long sys_cx_rng(uint8_t *buffer, unsigned int length);

unsigned long sys_io_seproxyhal_spi_is_status_sent(void);
unsigned long sys_io_seproxyhal_spi_send(const uint8_t *buffer, uint16_t length);
unsigned long sys_io_seproxyhal_spi_recv(uint8_t *buffer, uint16_t maxlength, unsigned int flags);

void unload_running_app(void);
int run_lib(char *name, unsigned long *parameters);

unsigned long sys_try_context_set(try_context_t *context);
unsigned long sys_try_context_get(void);

#endif
