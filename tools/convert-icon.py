#!/usr/bin/env python3

'''
Convert an image to a bytes array, as expected by SDL_CreateRGBSurfaceFrom.
This code is ripped off sdl2/ext/image.py.
'''

import binascii
import sys

from sdl2 import endian
from PIL import Image

def convert(fname):
    image = Image.open(fname)
    mode = image.mode
    width, height = image.size
    rmask = gmask = bmask = amask = 0
    if mode in ("1", "L", "P"):
        # 1 = B/W, 1 bit per byte
        # "L" = greyscale, 8-bit
        # "P" = palette-based, 8-bit
        pitch = width
        depth = 8
    elif mode == "RGB":
        # 3x8-bit, 24bpp
        if endian.SDL_BYTEORDER == endian.SDL_LIL_ENDIAN:
            rmask = 0x0000FF
            gmask = 0x00FF00
            bmask = 0xFF0000
        else:
            rmask = 0xFF0000
            gmask = 0x00FF00
            bmask = 0x0000FF
        depth = 24
        pitch = width * 3
    elif mode in ("RGBA", "RGBX"):
        # RGBX: 4x8-bit, no alpha
        # RGBA: 4x8-bit, alpha
        if endian.SDL_BYTEORDER == endian.SDL_LIL_ENDIAN:
            rmask = 0x000000FF
            gmask = 0x0000FF00
            bmask = 0x00FF0000
            if mode == "RGBA":
                amask = 0xFF000000
        else:
            rmask = 0xFF000000
            gmask = 0x00FF0000
            bmask = 0x0000FF00
            if mode == "RGBA":
                amask = 0x000000FF
        depth = 32
        pitch = width * 4
    else:
        # We do not support CMYK or YCbCr for now
        raise TypeError("unsupported image format")

    pxbuf = image.tobytes()

    print(binascii.hexlify(pxbuf), width, height, depth, pitch, rmask, gmask, bmask, amask)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: %s <file.png>' % sys.argv[0])
        sys.exit(1)

    convert(sys.argv[1])
