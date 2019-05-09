The goal of this project is to emulate Ledger Nano apps on standard desktop
computers. The emulator handles only a few syscalls made by common apps; for
instance, syscalls related to app install, firmware update, or OS info aren't
going to be implemented.

## Requirements

```console
sudo apt install qemu-user-static python3-pyqt5 python3-construct python3-mnemonic gcc-arm-linux-gnueabihf gdb-multiarch
```

The Python packages are used to emulate the device screen.


## Build

OpenSSL is currently required for Elliptic Curve cryptography operations. It
should be replaced by BOLOS code at some point. Meanwhile:

### OpenSSL

```console
git submodule update --init
cd openssl/
./Configure --cross-compile-prefix=arm-linux-gnueabihf- no-asm no-threads no-shared linux-armv4
make CFLAGS=-mthumb
```

### speculos

```console
make -C src/
```


## Usage

Basic usage:

```console
./speculos.py apps/btc.elf
```

### Bitcoin Testnet app

Launch the Bitcoin Testnet app, which requires the Bitcoin app:

```console
./speculos.py ./apps/btc-test.elf -l Bitcoin:./apps/btc.elf
```

### btchip-python

Use [btchip-python](https://github.com/LedgerHQ/btchip-python) without a real device:

```console
PYTHONPATH=$(pwd) LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 python tests/testMultisigArmory.py
```

### ledger-live-common

```console
./tools/ledger-live-http-proxy.py &
DEBUG_COMM_HTTP_PROXY=http://127.0.0.1:9998 ledger-live getAddress -c btc --path "m/49'/0'/0'/0/0" --derivationMode segwit
```

### Debug

Debug an app thanks to gdb:

```console
./speculos.py -d apps/btc.elf &
./tools/debug.sh apps/btc.elf
```


## Internals

The emulator is actually a userland application cross-compiled for the ARM
architecture. It opens the target app (`bin.elf`) from the filesystem and maps
it as is in memory. The emulator is launched with `qemu-arm-static` and
eventually jumps to the app entrypoint.

The `svc` instruction is replaced with `udf` (undefined) to generate a `SIGILL`
signal upon execution. It allows to catch syscalls and emulate them.

Apps can be debugged with `gdb-multiarch` thanks to `qemu-arm-static`.


## Syscall emulation

Apps don't use a lot of syscalls. For instance, here is the complete list of
syscalls used by the BTC app:

- system calls:
    - `check_api_level`
    - `reset`
    - `nvm_write`
    - `os_perso_derive_node_bip32`
    - `os_global_pin_is_validated`
    - `os_sched_exit`
    - `os_ux`
    - `os_lib_throw`
    - `os_flags`
    - `os_registry_get_current_app_tag`
- crypto syscalls, to be implemented using standard crypto lib:
    - `cx_rng`
    - `cx_hash`
    - `cx_ripemd160_init`
    - `cx_sha256_init`
    - `cx_blake2b_init2`
    - `cx_hmac_sha256`
    - `cx_ecfp_init_private_key`
    - `cx_ecfp_generate_pair`
    - `cx_ecdsa_sign`
    - `cx_ecdsa_verify`
- IO syscalls:
    - `io_seproxyhal_spi_send`
    - `io_seproxyhal_spi_is_status_sent`
    - `io_seproxyhal_spi_recv`
