#!/usr/bin/env python3

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
            #print('received: ' + outstanding[int(p.fields[3])])
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

seq_num = 1

def change_baud(new_baud):
    global seq_num
    p = ssrn.Packet()
    data ='$SRN|+999|-999|%04d|BAUD|%d' % (seq_num, new_baud)
    p.set_data(data)
    lock.acquire()
    outstanding[seq_num] = ('seq%d:' % seq_num) + data
    lock.release()
    network.send_timed(p)
    network.flush_output()
    num_nodes = 4
    time.sleep(10 * (len(p.data)+1) * (num_nodes+1) / network.get_baud())
    # ideally wait for packet to come back, with timeout
    #   if not, hard reset and try again
    network.set_baud(new_baud);
    seq_num = (seq_num + 1) % 10000

def blink_all():
    global seq_num
    p = ssrn.Packet()
    data ='$SRN|+999|-999|%04d|LED-BLINK' % (seq_num)
    p.set_data(data)
    lock.acquire()
    outstanding[seq_num] = ('seq%d:' % seq_num) + data
    lock.release()
    network.send_timed(p)
    seq_num = (seq_num + 1) % 10000

def read_all():
    global seq_num
    for i in range(1, 5):
        p = ssrn.Packet()
        data ='$SRN|+%03d|-999|%04d|READ' % (i, seq_num)
        p.set_data(data)
        lock.acquire()
        outstanding[seq_num] = ('seq%d:' % seq_num) + data
        lock.release()
        network.send_timed(p, 1.5)
        seq_num = (seq_num + 1) % 10000
        time.sleep(0.010)

def stats_all():
    global seq_num
    for i in range(1, 5):
        p = ssrn.Packet()
        data ='$SRN|+%03d|-999|%04d|PACKET-STATS' % (i, seq_num)
        p.set_data(data)
        lock.acquire()
        outstanding[seq_num] = ('seq%d:' % seq_num) + data
        lock.release()
        network.tx_queue.put(p)
        seq_num = (seq_num + 1) % 10000
        p = ssrn.Packet()
        data ='$SRN|%04d|-999|%04d|ERROR-STATS' % (i, seq_num)
        p.set_data(data)
        lock.acquire()
        outstanding[seq_num] = ('seq%d:' % seq_num) + data
        lock.release()
        network.tx_queue.put(p)
        seq_num = (seq_num + 1) % 10000

print('hard reset')
network.hard_reset()
time.sleep(1)

# setup signal handler to quit with current stats
quit_signal = False
def quit_handler(signum, frame):
    global quit_signal
    quit_signal = True

signal.signal(signal.SIGINT, quit_handler)
signal.siginterrupt(signal.SIGINT, False)

#baud_rates = [1200, 2400, 4800, 9600, 19200, 38400, 57600,
#             115200, 230400, 460800]

#baud_rates = [1200, 2400, 4800, 9600, 19200, 38400, 57600,
#             115200, 230400]

baud_rates = [1000, 2000, 3200, 5000, 8000,
              10000, 20000, 32000, 50000, 80000,
              100000, 200000]#, 320000, 500000, 800000, 1000000]

for baud in baud_rates:
    if quit_signal:
        break
    change_baud(baud)
    for i in range(3):
        if quit_signal:
            break
        blink_all()

try:
    for i in range(1000000):
        baud = random.choice(baud_rates)
        if quit_signal:
            break
        change_baud(baud)
        for i in range(3):
            if quit_signal:
                break
            blink_all()
            #if quit_signal:
            #    break
            #read_all()
except:
    pass

time.sleep(3)

stats_all()

time.sleep(4)

lock.acquire()
print()

missing = False
missing_count = 0
for i in outstanding:
    #print('missing:', outstanding[i])
    missing_count += 1
    missing = True
lock.release()
if not missing:
    print('all OK')
else:
    print('missing: %d' % missing_count)


    
