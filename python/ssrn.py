#!/usr/bin/env python3

import sys
import serial
import time
import threading
import queue

class Packet:
    ADDRESS_LOCALHOST = 0
    ADDRESS_BROADCAST = 999
    ADDRESS_CONTROLLER = -999
    
    def __init__(self, data=None):
        if data:
            self.set_data(data)

    @property
    def dst_address(self):
        return int(self.fields[1])

    @property
    def src_address(self):
        return int(self.fields[2])

    def modify_addresses(self):
        dst = self.dst_address
        if dst > -998 and dst < 999:
            dst -= 1
        src = self.src_address
        if src > -999 and src < 998:
            src += 1
        self.dst_address = dst
        self.src_address = src
        self.fields[1] = '+03d' % dst
        self.fields[2] = '+03d' % src
        self.data = '|'.join(self.fields[:-1])
        self.append_checksum()
        
    def calculate_checksum(self):
        sum = 0
        for c in self.data:
            if c == '*':
                break
            if c != '|':
                sum += ord(c)
        sum &= 0xff
        return sum
    
    def append_checksum(self):
        sum = self.calculate_checksum()
        self.data += '|*%02X' % sum
        
    def validate_checksum(self):
        cs = int(self.fields[-1][1:3], 16)
        sum = self.calculate_checksum()
        if cs == sum:
            return True
        else:
            return False
        
    def decode(self, data):
        self.data = data
        self.fields = data.split('|')
        #print(self.fields)
        if self.fields[0] == '$SRN' and self.validate_checksum():
            return True
        else:
            return False
        
    def set_data(self, data):
        self.data = data
        self.append_checksum()
        self.fields = self.data.split('|')        

def _transmit_task(port, queue):
    while True:
        packet = queue.get()
        #print('tx: ', packet.data)
        port.write((packet.data + '\n').encode('ascii'))

def _receive_task(port, queue):
    while True:
        try:
            raw = port.read_until(b'\n')
            data = raw.decode('ascii', errors='strict').rstrip()
        except UnicodeDecodeError:
            print('decode error: ', raw)
            continue
        #print('rx: ' + data)
        packet = Packet()
        if packet.decode(data):
            #print('rx: ', packet.data)
            queue.put(packet)
        else:
            print('rx error:', data)

def _forwarding_task(input_queue, rx_queue, tx_queue):
    while True:
        packet = rx_queue.get()
        if packet.dst_address == Packet.ADDRESS_LOCALHOST:
            input_queue.put(packet)
        elif packet.dst_address == Packet.ADDRESS_BROADCAST:
            input_queue.put(packet)
            tx_queue.put(packet)
        else:
            packet.modify_addresses()
            tx_queue.put(packet)

class SSRN:
    CONTROLLER = 0
    NODE = 1
    def __init__(self, port, type=NODE):
        self.port = port
        self.type = type
        self.tx_queue = queue.Queue()
        self.rx_queue = queue.Queue()
        self.input_queue = queue.Queue()
            
        self.tx_thread = threading.Thread(target=_transmit_task,
                                          args=(self.port,
                                                self.tx_queue),
                                          daemon=True)
        self.rx_thread = threading.Thread(target=_receive_task,
                                          args=(self.port,
                                                self.rx_queue),
                                          daemon=True)

        if self.type == SSRN.NODE:
            self.fwd_thread = threading.Thread(target=_forwarding_task,
                                               args=(self.input_queue,
                                                     self.rx_queue,
                                                     self.tx_queue),
                                               daemon=True)
    def start(self):
        self.tx_thread.start()
        self.rx_thread.start()
        if self.type == SSRN.NODE:
            self.fwd_thread.start()

    def get_baud(self):
        return self.port.baudrate

    def set_baud(self, baud):
        self.port.baudrate = baud

    def discard_input(self):
        self.port.reset_input_buffer();

    def flush_output(self):
        while not self.tx_queue.empty():
            time.sleep(0.050)
        self.port.flush()

    def send_timed(self, packet, ratio=-1):
        self.tx_queue.put(packet)
        if ratio < 1:
            max_bits = 100 * 10 # !!! hard-coded max packet length
        else:
            max_bits = 10*(len(packet.data)+1)*ratio
        delay = max_bits/self.port.baudrate
        #print(delay)
        time.sleep(delay)
        

# !!! to-do: create exception classes for network
    def hard_reset(self):
        if not self.port.dsr:
            print('dsr not high')
            raise IOError
        self.port.dtr = 0
        time.sleep(0.001)
        if self.port.dsr:
            print('dsr not low')
            raise IOError
        self.port.dtr = 1
        if not self.port.dsr:
            print('dsr not high')
            raise IOError

