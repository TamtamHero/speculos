import binascii
import os
import select
import signal
import sys
import time

from . import apdu
from . import screen

SEPROXYHAL_TAG_BUTTON_PUSH_EVENT = 0x05
SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT = 0x0d
SEPROXYHAL_TAG_CAPDU_EVENT = 0x16

SEPROXYHAL_TAG_RAPDU = 0x53
SEPROXYHAL_TAG_GENERAL_STATUS = 0x60
SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND = 0x0000
SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS = 0x65

class ServiceExit(Exception):
    '''
    Custom exception which is used to trigger the clean exit of all running
    threads and the main program.
    '''

class SeProxyHal:
    def __init__(self, s, color='MATTE_BLACK', headless=False):
        self.s = s
        self.headless = headless

        self.stop_flag = False
        signal.signal(signal.SIGTERM, self._sighandler)
        signal.signal(signal.SIGINT, self._sighandler)

        self.pipe_r, pipe_w = os.pipe()

        if not self.headless:
            self.display_thread = screen.DisplayThread(pipe_w, color)
        else:
            self.display_thread = screen.HeadlessDisplayThread(pipe_w, color)

        self.display_thread.start()

        self.apdu_thread = apdu.ApduThread(callback=self._apdu_callback)
        self.apdu_thread.start()

    def _sighandler(self, signum, frame):
        self._stop_flag = True
        raise ServiceExit

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
        self.s.sendall(packet)

    def _handle_seph_packet(self, display):
        '''Handle packet sent by app.'''

        data = self._recvall(3)
        tag = data[0]
        size = int.from_bytes(data[1:3], 'big')

        data = self._recvall(size)
        assert len(data) == size

        #print('[*] seproxyhal: received (tag: 0x%02x, size: 0x%02x): %s' % (tag, size, repr(data)), file=sys.stderr)

        if tag == SEPROXYHAL_TAG_GENERAL_STATUS:
            if int.from_bytes(data[:2], 'big') == SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND:
                display.screen_update()

        elif tag == SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS:
            #print('[*] seproxyhal: DISPLAY_STATUS %s' % repr(data), file=sys.stderr)
            display.display_status(data)
            self._send_packet(SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT)

        elif tag == SEPROXYHAL_TAG_RAPDU:
            self.apdu_thread.from_device(data)

        elif tag in [ 0x4f, 0x50 ]:
            pass

        else:
            print('unknown tag: 0x%x' % tag)
            sys.exit(0)

    def _handle_buttons(self, buttons):
        '''Forward button press/release from the GUI to the app.'''

        for button, pressed in buttons:
            if pressed:
                self._send_packet(SEPROXYHAL_TAG_BUTTON_PUSH_EVENT, (button << 1).to_bytes(1, 'big'))
            else:
                self._send_packet(SEPROXYHAL_TAG_BUTTON_PUSH_EVENT, (0 << 1).to_bytes(1, 'big'))

    def _apdu_callback(self, packet):
        '''Forward raw APDU to the app.'''

        self._send_packet(SEPROXYHAL_TAG_CAPDU_EVENT, packet)

    def _seproxyhal_server(self):
        #while self.display_thread.is_alive() and self.apdu_thread.is_alive():
        while not self.stop_flag:
            r, _, _ = select.select([ self.s, self.pipe_r ], [], [])

            if self.s in r:
                self._handle_seph_packet(self.display_thread.display)
            elif self.pipe_r in r:
                data = os.read(self.pipe_r, 4096)
                if data is None:
                    break
                buttons = self.display_thread.display.handle_events()
                self._handle_buttons(buttons)

    def seproxyhal_server(self):
        try:
            self._seproxyhal_server()
        except ServiceExit:
            for thread in [self.display_thread, self.apdu_thread]:
                thread.stop()

            for thread in [self.display_thread, self.apdu_thread]:
                thread.join()
