#!/usr/bin/env python3

"""
command line control for SSRN
"""
import cmd
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

num_nodes = -1

def seq_num():
    seq_num.counter = (seq_num.counter + 1) % 10000
    return seq_num.counter
seq_num.counter = 0

def hard_reset():
    network.hard_reset()
    network.set_baud(1200)
    time.sleep(1)
    print('ready')

def change_baud(new_baud):
    p = ssrn.Packet('$SRN|+999|-999|%04d|BAUD|%d' % (seq_num(), new_baud))
    network.send_timed(p)
    network.flush_output()
    try:
        response = network.rx_queue.get(block=True)
    except KeyboardInterrupt:
        print('aborted: resetting')
        hard_reset()
        return
    print(response.data)
    time.sleep(10 * (len(p.data)+1) * (num_nodes+1) / network.get_baud())
    network.set_baud(new_baud)

def send_receive(request, dst_addr):
    packet = ssrn.Packet(request % (dst_addr, seq_num()))
    network.send_timed(packet)
    try:
        response = network.rx_queue.get(block=True)
    except KeyboardInterrupt:
        print('aborted')
        return
    print(response.data)

def send_all(request):
    address = 1
    while True:
        packet = ssrn.Packet(request % (address, seq_num()))
        network.send_timed(packet)
        try:
            response = network.rx_queue.get(block=True)
        except KeyboardInterrupt:
            print('aborted')
            return
        fields = response.data.split('|')
        if int(fields[1]) != -999:
            return
        else:
            print(response.data)
        address += 1

def node_voltage(address):
    request = '$SRN|+%03d|-999|%04d|VOLTAGE'
    packet = ssrn.Packet(request % (address, seq_num()))
    network.send_timed(packet)
    try:
        response = network.rx_queue.get(block=True)
    except KeyboardInterrupt:
        print('aborted')
        return
    fields = response.data.split('|')
    if fields[4] == 'UNKNOWN':
        return -1
    else:
        voltage = float(fields[5])
        return voltage
    

def ssrn_discovery():
    global num_nodes
    address = 1
    done = False
    print()
    print(' address |        node-type       | node-id | voltage ')
    print('---------|------------------------|---------|---------')
    while not done:
        request = '$SRN|+%03d|-999|%04d|PING'
        packet = ssrn.Packet(request % (address, seq_num()))
        network.send_timed(packet)
        try:
            response = network.rx_queue.get(block=True)
        except KeyboardInterrupt:
            print('aborted')
            return
        fields = response.data.split('|')
        if int(fields[1]) != -999:
            done = True
        else:
            node_type = fields[5]
            node_ver = fields[6]
            voltage = node_voltage(address)
            if voltage >= 0:
                print('     %03d | % 22s | % 7s |  %5.3f' %
                      (address, node_type, node_ver, voltage))
            else:
                print('     %03d | % 22s | % 7s |  -----' %
                      (address, node_type, node_ver))
        address += 1
    num_nodes = address - 2
    print()
    print('%d nodes discovered' % num_nodes)
    print()
        
class SSRN_Shell(cmd.Cmd):
    intro = 'SSRN Command Shell\n'
    prompt = 'SSRN> '
    def preloop(self):
        network.hard_reset()
        network.set_baud(1200)
        time.sleep(1)
    def do_quit(self, arg):
        sys.exit(0)
    def do_q(self, arg):
        sys.exit(0)
    def do_exit(self, arg):
        sys.exit(0)
    def do_reset(self, arg):
        hard_reset()
    def do_discover(self, arg):
        ssrn_discovery()
    def do_baud(self, arg):
        new_baud = int(arg)
        change_baud(new_baud)
    def do_delay(self, arg):
        duration = float(arg)
        time.sleep(duration)
    def default(self, arg):
        fields = arg.upper().split()
        if len(fields) == 2:
            request = '$SRN|+%03d|-999|%04d|' + fields[0]
            if fields[1] == 'all' or fields[1] == 'ALL':
                send_all(request)
            else:
                address = int(fields[1])
                send_receive(request, address)
        elif len(fields) == 3:
            try:
                repeat_count = int(fields[2])
                request = '$SRN|+%03d|-999|%04d|' + fields[0]
                if fields[1] == 'all' or fields[1] == 'ALL':
                    for i in range(repeat_count):
                        send_all(request)
                else:
                    address = int(fields[1])
                    for i in range(repeat_count):
                        send_receive(request, address)
            except KeyboardInterrupt:
                pass
        else:
            print('syntax error')

if __name__ == '__main__':
    SSRN_Shell().cmdloop()
