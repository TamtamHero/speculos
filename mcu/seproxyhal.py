import binascii
import os
import select
import sys
import time

from . import apdu
from . import screen

SEPROXYHAL_TAG_BUTTON_PUSH_EVENT = 0x05
SEPROXYHAL_TAG_FINGER_EVENT = 0x0c
SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT = 0x0d
SEPROXYHAL_TAG_TICKER_EVENT = 0x0e
SEPROXYHAL_TAG_CAPDU_EVENT = 0x16

SEPROXYHAL_TAG_RAPDU = 0x53
SEPROXYHAL_TAG_GENERAL_STATUS = 0x60
SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND = 0x0000
SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS = 0x65
SEPROXYHAL_TAG_PRINTF_STATUS = 0x66
SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS = 0x69

SEPROXYHAL_TAG_FINGER_EVENT_TOUCH   = 0x01
SEPROXYHAL_TAG_FINGER_EVENT_RELEASE = 0x02

class ServiceExit(Exception):
    '''
    Custom exception which is used to trigger the clean exit of all running
    threads and the main program.
    '''

class SeProxyHal:
    def __init__(self, s):
        self.s = s

    def _recvall(self, size):
        data = b''
        while size > 0:
            tmp = self.s.recv(size)
            if len(tmp) == 0:
                print('[-] seproxyhal: fd closed', file=sys.stderr)
                sys.exit(1)
            data += tmp
            size -= len(tmp)
        return data

    def _send_packet(self, tag, data=b''):
        '''Send packet to the app.'''

        size = len(data).to_bytes(2, 'big')
        packet = tag.to_bytes(1, 'big') + size + data
        #print('[*] seproxyhal: send %s\n' % binascii.hexlify(packet), file=sys.stderr)
        try:
            self.s.sendall(packet)
        except BrokenPipeError:
            # the pipe is closed, which means the app exited
            raise ServiceExit

    def can_read(self, s, screen):
        '''
        Handle packet sent by the app.

        This function is called thanks to a screen QSocketNotifier.
        '''

        assert s == self.s.fileno()

        data = self._recvall(3)
        tag = data[0]
        size = int.from_bytes(data[1:3], 'big')

        data = self._recvall(size)
        assert len(data) == size

        #print('[*] seproxyhal: received (tag: 0x%02x, size: 0x%02x): %s' % (tag, size, repr(data)), file=sys.stderr)

        if tag == SEPROXYHAL_TAG_GENERAL_STATUS:
            if int.from_bytes(data[:2], 'big') == SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND:
                screen.screen_update()

        elif tag == SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS:
            #print('[*] seproxyhal: DISPLAY_STATUS %s' % repr(data), file=sys.stderr)
            screen.display_status(data)
            self._send_packet(SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT)
            screen.screen_update()


        elif tag == SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS:
            #print('SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS', file=sys.stderr)
            screen.display_raw_status(data)
            self._send_packet(SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT)
            screen.screen_update()

        elif tag == SEPROXYHAL_TAG_RAPDU:
            screen.forward_to_apdu_client(data)

        elif tag == SEPROXYHAL_TAG_PRINTF_STATUS:
            for b in data:
                sys.stdout.write('%c' % chr(b))
            sys.stdout.flush()
            self._send_packet(SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT)

        elif tag in [ 0x4f, 0x50 ]:
            pass

        else:
            print('unknown tag: 0x%x' % tag)
            sys.exit(0)

    def handle_button(self, button, pressed):
        '''Forward button press/release from the GUI to the app.'''

        if pressed:
            self._send_packet(SEPROXYHAL_TAG_BUTTON_PUSH_EVENT, (button << 1).to_bytes(1, 'big'))
        else:
            self._send_packet(SEPROXYHAL_TAG_BUTTON_PUSH_EVENT, (0 << 1).to_bytes(1, 'big'))

    def handle_finger(self, x, y, pressed):
        '''Forward finger press/release from the GUI to the app.'''

        if pressed:
            packet = SEPROXYHAL_TAG_FINGER_EVENT_TOUCH.to_bytes(1, 'big')
        else:
            packet = SEPROXYHAL_TAG_FINGER_EVENT_RELEASE.to_bytes(1, 'big')
        packet += x.to_bytes(2, 'big')
        packet += y.to_bytes(2, 'big')

        self._send_packet(SEPROXYHAL_TAG_FINGER_EVENT, packet)

    def to_app(self, packet):
        '''Forward raw APDU to the app.'''

        self._send_packet(SEPROXYHAL_TAG_CAPDU_EVENT, packet)
