import binascii
import os
import select
import socket
import sys
import threading

class DefaultFraming:
    '''Raw APDUs'''

    def __init__(self, _recvall, _sendall):
        self._recvall = _recvall
        self._sendall = _sendall

    def remove_framing(self, packet):
        pass

    def recv_packet(self):
        data = self._recvall(4)
        if data is None:
            return None

        size = int.from_bytes(data, byteorder='big')
        packet = self._recvall(size)
        if packet is None:
            return None

        return packet

    def send_packet(self, packet):
        size = len(packet) - 2
        packet = size.to_bytes(4, 'big') + packet
        self._sendall(packet)

class HidFraming:
    '''Ledger Live'''

    def __init__(self, _recvall, _sendall, packet_size=64):
        self.channel = None
        self.tag = None
        self._recvall = _recvall
        self._sendall = _sendall
        self.packet_size = packet_size

        self._reset_acc()

    def _reset_acc(self):
        self.acc = None
        self.seq = 0
        self.data = b''
        self.data_length = 0

    def _remove_framing(self, packet):
        '''
        Remove HID framing and return the actual data.
        '''

        packet = packet[1:] # skip first byte
        channel = int.from_bytes(packet[:2], 'big')
        tag = int.from_bytes(packet[2:3], 'big')
        seq = int.from_bytes(packet[3:5], 'big')

        if self.channel is None and self.tag is None:
            self.channel = channel
            self.tag = tag

        if self.acc is None:
            self.data_length = int.from_bytes(packet[5:7], 'big')
            data = packet[7:]
            print(self.acc, self.data_length, repr(packet[5:]))
        else:
            data = packet[5:]

        if len(self.data) + len(data) > self.data_length:
            data = data[:self.data_length-len(self.data)]

        self.data += data
        self.seq += 1

        if len(self.data) == self.data_length:
            self._reset_acc()

        return data

    def recv_packet(self):
        data = self._recvall(self.packet_size)
        if data is None:
            return None

        return self._remove_framing(data)

    def send_packet(self, apdu):
        data = len(apdu).to_bytes(2, 'big') + apdu
        block_size = self.packet_size - 5

        seq = 0
        while len(data) > 0:
            packet  = self.channel.to_bytes(2, 'big')
            packet += self.tag.to_bytes(1, 'big')
            packet += seq.to_bytes(2, 'big')
            packet += data[:block_size]

            data = data[block_size:]

            if len(packet) != self.packet_size:
                packet = packet.ljust(self.packet_size, b'\x00')

            self._sendall(packet)

            seq += 1

class ApduThread(threading.Thread):
    '''
    Forward packets between an external application and the emulated
    device.

    Internally, it makes use of a TCP socket server to allow these 2
    components to communicate.
    '''

    def __init__(self, callback, host='127.0.0.1', port=9999, hid=False):
        threading.Thread.__init__(self)

        self._stop_event = threading.Event()

        self.callback = callback

        if hid == False:
            framing = DefaultFraming
        else:
            framing = HidFraming

        self.framing = framing(lambda x: self._recvall(x), lambda x: self.c.sendall(x))

        # pipe for the stop method
        self.exit_pipe_r, self.exit_pipe_w = os.pipe()

        self.c = None
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind((host, port))
        self.s.listen()

    def _recvall(self, size):
        data = b''
        while size > 0:
            try:
                tmp = self.c.recv(size)
            except ConnectionResetError:
                tmp = b''
            if len(tmp) == 0:
                print("[-] apduthread: connection with client closed", file=sys.stderr)
                return None
            data += tmp
            size -= len(tmp)
        return data

    def _accept(self):
        '''Return True if a client connected successfully, False otherwise.'''

        r, _, _ = select.select([ self.s, self.exit_pipe_r ], [], [])
        if self.exit_pipe_r in r:
            return False

        self.c, _ = self.s.accept()

        return True

    def stop(self):
        '''This method is called by the main thread.'''

        self._stop_event.set()

        # This thread should be waiting in a select call. Close the exit pipe to
        # make select return.
        os.close(self.exit_pipe_w)

        if self.c:
            self.c.close()
            self.c = None

    def from_device(self, packet):
        '''Encode and forward APDU to the client.'''

        if self.c is None:
            print("[-] apduthread: client doesn't exist yet, skipping packet", file=sys.stderr)
            return

        print('[*] apdupthread: RAPDU: %s' % binascii.hexlify(packet), file=sys.stderr)

        self.framing.send_packet(packet)

    def to_device(self, packet):
        if self.c is None:
            print("[-] apduthread: client doesn't exist yet, skipping packet", file=sys.stderr)
            return

        self.callback(packet)

    def _handle_client(self):
        if not self._accept():
            self.s.close()
            self.s = None
            return False

        while not self._stop_event.is_set():
            r, _, _ = select.select([ self.c, self.exit_pipe_r ], [], [])
            if self.exit_pipe_r in r:
                break

            packet = self.framing.recv_packet()
            if packet is None:
                break

            print('[*] packet', repr(packet))
            self.to_device(packet)

        return True

    def run(self):
        while self._handle_client():
            pass
