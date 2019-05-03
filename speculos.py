#!/usr/bin/env python3

'''
Emulate the target app along the SE Proxy Hal server.
'''

import argparse
import binascii
import ctypes
from mnemonic import mnemonic
import os
import signal
import socket
import sys
import time

from mcu import screen, seproxyhal

DEFAULT_SEED = 'glory promote mansion idle axis finger extra february uncover one trip resource lawn turtle enact monster seven myth punch hobby comfort wild raise skin'

def set_pdeath(sig):
    '''Set the parent death signal of the calling process.'''

    PR_SET_PDEATHSIG = 1
    libc = ctypes.cdll.LoadLibrary('libc.so.6')
    libc.prctl(PR_SET_PDEATHSIG, sig)

def run_qemu(s1, s2, app_path, libraries=[], seed=DEFAULT_SEED, debug=False):
    args = [ 'qemu-arm-static' ]
    if debug:
        args += [ '-g', '1234', '-singlestep' ]
    args += [ './src/launcher', app_path ]
    if libraries:
        args += libraries

    pid = os.fork()
    if pid != 0:
        return

    # ensure qemu is killed when this Python script exits
    set_pdeath(signal.SIGTERM)

    s2.close()

    # replace stdin with the socket
    os.dup2(s1.fileno(), sys.stdin.fileno())

    seed = mnemonic.Mnemonic.to_seed(seed)
    os.environ['SPECULOS_SEED'] = binascii.hexlify(seed).decode('ascii')

    #print('[*] seproxyhal: executing qemu', file=sys.stderr)
    os.execvp(args[0], args)

    sys.exit(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Emulate Ledger Nano apps.')
    parser.add_argument('app.elf', type=str, help='application path')
    parser.add_argument('--color', default='MATTE_BLACK', choices=list(screen.Bagl.COLORS.keys()), help='Nano color')
    parser.add_argument('-d', '--debug', action='store_true', help='Wait gdb connection to port 1234')
    parser.add_argument('-l', '--library', action='append', help='Additional library (eg. Bitcoin:app/btc.elf) which can be called through os_lib_call')
    parser.add_argument('-n', '--headless', action='store_true', help="Don't display the GUI")
    parser.add_argument('-s', '--seed', action='store_true', default=DEFAULT_SEED, help='Seed')
    args = parser.parse_args()

    s1, s2 = socket.socketpair()

    run_qemu(s1, s2, getattr(args, 'app.elf'), args.library, args.seed, args.debug)
    s1.close()

    seph = seproxyhal.SeProxyHal(s2, color=args.color, headless=args.headless)
    seph.seproxyhal_server()
    s2.close()
