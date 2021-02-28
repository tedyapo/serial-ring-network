#!/usr/bin/env python3

import sys
import serial
import time
import threading
import signal
import ssrn

port_device = sys.argv[1]
port = serial.Serial(port_device, 1200)

'''
class AM2302_Node:
    def __init__(self):
        pass
    def request(self, address, seq_num):
        self.seq_num = seq_numa
        return 'SSRN,%d,0,%d,READ' % (address, seq_num)
        pass
    def response(self, packet):
        self.
        pass
'''

'''
!!
note: all nodes should support LED_ON, LED_OFF messages
maybe add LED_BLINK

SSRN, dst_addr, src_addr, seq_num, LED_ON
SSRN, dst_addr, src_addr, seq_num, LED_OFF
SSRN, dst_addr, src_addr, seq_num, LED_BLINK

response:
SSRN, dst_addr, src_addr, seq_num, OK
SSRN, dst_addr, src_addr, seq_num, ERROR, <error number>

!!
'''

lock = threading.Lock()
outstanding = {}

def processing_task(input_queue):
    while True:
        p = input_queue.get()
        print('processing :', p.data)
        if (p.fields[4] == 'PACKET-STATS'):
            print('received_count = ', p.fields[5])
            print('forwarded_count = ', p.fields[6])
            print('inbound_count = ', p.fields[7]);
            print('transmitted_count = ', p.fields[8]);
        if (p.fields[4] == 'ERROR-STATS'):
            print('rx_error_count = ', p.fields[5])
            print('invalid_packet_count = ', p.fields[6])
            print('forward_drop_count = ', p.fields[7])
            print('inbound_drop_count = ', p.fields[8]);
            print('tx_error_count = ', p.fields[9]);
        lock.acquire()

        key = int(p.fields[3])
        if key in outstanding:
            print('received: ' + outstanding[int(p.fields[3])])
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

class netstat:
    def __init__(self):
        pass
    def request(self, dst_addr, seq_num):
        return '$SRN|%+03d|-999|%04d|NETSTAT' % (dst_addr, seq_num)
    def response_handler(self, data):
        pass

print('hard reset')
network.hard_reset()
time.sleep(1)

print('sending')

p = ssrn.Packet()
data ='$SRN|+001|-999|%04d|RESET' % -2
p.set_data(data)
network.tx_queue.put(p)

time.sleep(2)


p = ssrn.Packet()
data ='$SRN|+999|-999|%04d|BAUD|115200' % -3
p.set_data(data)
lock.acquire()
outstanding[-3] = ('seq%d:' % -3) + data
lock.release()
network.tx_queue.put(p)
network.flush_output()

time.sleep(1)
network.set_baud(115200);


p = ssrn.Packet()
data ='$SRN|+001|-999|%04d|RESET-STATS' % -2
p.set_data(data)
lock.acquire()
outstanding[-2] = ('seq%d:' % -2) + data
lock.release()
network.tx_queue.put(p)

time.sleep(0.25)

missing = False
missing_count = 0

# setup signal handler to quit with current stats
quit_signal = False
def quit_handler(signum, frame):
    global quit_signal
    quit_signal = True

signal.signal(signal.SIGINT, quit_handler)

for seq_num in range(10000000):
#for seq_num in range(10000):
    if quit_signal:
        break
    p = ssrn.Packet()
    if seq_num % 2:
        data = '$SRN|+001|-999|%04d|READ' % (seq_num % 10000)
    else:
        data ='$SRN|+001|-999|%04d|LED-BLINK' % (seq_num % 10000)
    p.set_data(data)
    lock.acquire()
    key = seq_num % 10000
    if key in outstanding:
        missing = True
        missing_count += 1
    outstanding[key] = ('seq%d:' % key) + data
    lock.release()
    network.send_timed(p, 1.7)

time.sleep(0.25)

p = ssrn.Packet()
data ='$SRN|+001|-999|%04d|PACKET-STATS' % -1
p.set_data(data)
lock.acquire()
outstanding[-1] = ('seq%d:' % -1) + data
lock.release()
network.tx_queue.put(p)

time.sleep(0.25)

p = ssrn.Packet()
data ='$SRN|+001|-999|%04d|ERROR-STATS' % -4
p.set_data(data)
lock.acquire()
outstanding[-4] = ('seq%d:' % -4) + data
lock.release()
network.tx_queue.put(p)

time.sleep(3)

lock.acquire()
print()

for i in outstanding:
    print('missing:', outstanding[i])
    missing_count += 1
    missing = True
lock.release()
if not missing:
    print('all OK')
else:
    print('missing: %d' % missing_count)

#t_proc.join()
    
