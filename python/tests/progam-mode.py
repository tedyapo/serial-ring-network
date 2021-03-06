#!/usr/bin/env python3

"""
open port to drop reset line for ISCP
"""

import sys
import serial
import time

sys.path.insert(1, '../')
import ssrn

port_device = sys.argv[1]
port = serial.Serial(port_device, 1200)

while True:
    time.sleep(10)
