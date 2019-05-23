AR := arm-linux-gnueabihf-ar
CC := arm-linux-gnueabihf-gcc
CFLAGS := -I../sdk -I../openssl/include -mthumb -W -Wall -fPIC -DOS_LITTLE_ENDIAN -DNATIVE_64BITS
LDFLAGS := -static -L../openssl -lssl -lcrypto -ldl

OBJS = \
	cx_aes.o \
	cx_blake2b.o \
	cx_crc.o \
	cx_ec.o \
	cx_hmac.o \
	cx_ripemd160.o \
	cx_scc.o \
	cx_sha256.o \
	cx_sha512.o \
	cx_utils.o \
	emulate.o \
	emu_cx.o \
	emu_cx_hash.o	\
	emu_cx_math.o \
	emu_default.o \
	emu_os.o \
	emu_os_bip32.o \
	emu_seproxyhal.o \
	launcher.o

all: launcher libemu.a

launcher: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

libemu.a: $(filter-out launcher.o,$(OBJS))
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm -f *.o *.a launcher
