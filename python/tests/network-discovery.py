#!/usr/bin/env python3

"""
SSRN network discovery tests
"""

import sys
import serial
import time
import threading
import signal
import random

sys.path.insert(1, '../')
import ssrn

port_device = sys.argv[1]
port = serial.Serial(port_device, 1200)

class Message:
    def __init__(self, sent_data=None, dst_addr=None, callback=None):
        self.sent_data = sent_data
        self.dst_addr = dst_addr
        self.callback = callback
        self.received_data = None
        self.packet = None

lock = threading.Lock()
outstanding = {}

def processing_task(input_queue):
    while True:
        p = input_queue.get()
        print('processing :', p.data)
        if (p.fields[4] == 'PACKET-STATS'):
            try:
                print('received_count = ', p.fields[5])
                print('forwarded_count = ', p.fields[6])
                print('inbound_count = ', p.fields[7]);
                print('transmitted_count = ', p.fields[8]);
            except:
                pass
        if (p.fields[4] == 'ERROR-STATS'):
            try:
                print('rx_error_count = ', p.fields[5])
                print('invalid_packet_count = ', p.fields[6])
                print('forward_drop_count = ', p.fields[7])
                print('inbound_drop_count = ', p.fields[8]);
                print('tx_error_count = ', p.fields[9]);
            except:
                pass

        lock.acquire()
        key = int(p.fields[3])
        if key in outstanding:
            outstanding[key].received_data = p.data
            outstanding[key].packet = p
            if outstanding[key].callback:
                outstanding[key].callback(outstanding[key])
            outstanding.pop(key, None)
        else:
            print('erroneous packet: ' + p.data)
        lock.release()

network = ssrn.SSRN(port, type=ssrn.SSRN.CONTROLLER)
t_proc = threading.Thread(target=processing_task,
                          args=(network.rx_queue,),
                          daemon=True)
t_proc.start()

network.start()

nodes = {}
discovery_done = False

def ping_callback(message):
    global discovery_done
    print(int(message.packet.fields[1]))
    print(ssrn.Packet.ADDRESS_CONTROLLER)
    if int(message.packet.fields[1]) == ssrn.Packet.ADDRESS_CONTROLLER:
        nodes[message.dst_addr] = message.packet.fields[5]
    else:
        discovery_done = True


print('hard reset')
network.hard_reset()
time.sleep(1)

seq_num = 1
dst_addr = 1
while not discovery_done:
    p = ssrn.Packet('$SRN|+%03d|-999|%04d|PING' % (dst_addr, seq_num))
    lock.acquire()
    outstanding[seq_num] = Message(sent_data=p.data,
                                   dst_addr=dst_addr,
                                   callback=ping_callback)
    lock.release()
    network.send_timed(p)
    time.sleep(1)
    dst_addr += 1
    seq_num += 1

print(nodes)
