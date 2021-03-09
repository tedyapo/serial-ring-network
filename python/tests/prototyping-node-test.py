#!/usr/bin/env python3

"""
tests for SSRN prototyping node with PIC18F16Q41
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

network = ssrn.SSRN(port, type=ssrn.SSRN.CONTROLLER)

network.start()

print('hard reset')
network.hard_reset()
time.sleep(1)

"""
nodes = {}

dst_addr = 1
while True:
    request = ssrn.Packet('$SRN|+%03d|-999|%04d|PING' % (dst_addr, dst_addr))
    network.send_timed(request)
    response = network.rx_queue.get(timeout=1)
    if int(response.fields[1]) == ssrn.Packet.ADDRESS_CONTROLLER:
        nodes[dst_addr] = response.fields[5]
    else:
        break
    dst_addr += 1

print(nodes)
"""


seq_num = 1
new_baud = 115200
p = ssrn.Packet('$SRN|+999|-999|%04d|BAUD|%d' % (seq_num, new_baud))
network.send_timed(p)
network.flush_output()
response = network.rx_queue.get(timeout=1)
print(response.data)
num_nodes = 4
#time.sleep(10 * (len(p.data)+1) * (num_nodes+1) / network.get_baud())
network.set_baud(new_baud);

dst_addr = 4


request = ssrn.Packet('$SRN|+%03d|-999|%04d|WPU|0xff' % (dst_addr, dst_addr))
network.send_timed(request)
response = network.rx_queue.get(block=True, timeout=1)
print(response.data)
time.sleep(1)

request = ssrn.Packet('$SRN|+%03d|-999|%04d|TRIS|0x00' % (dst_addr, dst_addr))
network.send_timed(request)
response = network.rx_queue.get(block=True, timeout=1)
print(response.data)


# loopback test
while True:
    i = random.randint(0, 2**32)
    request = ssrn.Packet('$SRN|+%03d|-999|%04d|LOOPBACK|0x%x' %
                          (dst_addr, dst_addr, i))
    network.send_timed(request)
    response = network.rx_queue.get(timeout=1)
    value = int(response.fields[5])
    print(request.data)
    print(response.data)
    if i != value:
        print('error')
        sys.exit(-1)


while True:
    for i in range(256):
        request = ssrn.Packet('$SRN|+%03d|-999|%04d|WRITE|0x%x' %
                              (dst_addr, dst_addr, i))
        network.send_timed(request)
        response = network.rx_queue.get(timeout=1)
        print(response.data)
        #time.sleep(0.1)

while True:
    pass
