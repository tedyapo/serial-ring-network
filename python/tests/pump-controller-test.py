#!/usr/bin/env python3

"""
tests for SSRN pump controller node with PIC18F16Q41
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


seq_num = 1
new_baud = 115200
p = ssrn.Packet('$SRN|+999|-999|%04d|BAUD|%d' % (seq_num, new_baud))
network.send_timed(p)
network.flush_output()
response = network.rx_queue.get(timeout=1)
print(response.data)
num_nodes = 1
#time.sleep(10 * (len(p.data)+1) * (num_nodes+1) / network.get_baud())
network.set_baud(new_baud);

dst_addr = 1
request = ssrn.Packet('$SRN|+%03d|-999|%04d|PUMP2|50|20000' % (dst_addr, seq_num))
network.send_timed(request)
response = network.rx_queue.get(block=True, timeout=60)
print(response.data)
time.sleep(20)
