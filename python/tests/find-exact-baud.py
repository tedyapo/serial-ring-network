#!/usr/bin/env python3

"""
find matching exact (or close) baus rates on FT231X and PIC at 32 MHz
"""

import math

def closest_pic_baud(baud):
    n = round(32000000 / (4*baud) - 1)
    rate = 32000000 / (4*(n+1))
    err = abs(rate - baud)
    m = n
    r = 32000000 / (4*(n+2))
    """
    e = abs(r - baud)
    if e < err:
        err = e
        rate = r
        m = n + 1
    r = 32000000 / (4*(n))
    e = abs(r - baud)
    if e < err:
        err = e
        rate = r
        m = n - 1
    """
    return rate, m

def pic_baud(n):
    rate = 32000000 / (4*n+1)
    return rate

def closest_ft231_baud(baud):
    m = 3000000 / baud
    n = math.floor(m)
    if n < 2:
        n = 2
    x = int((m - n) * 8)/8
    if x < 0:
        x = 0
    rate = 3000000 / (n + x)
    return rate, n, x

for n in range(2, 16384):
    for x in [0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875]:
        baud = 3000000 / (n+x)
        baud2, k = closest_pic_baud(baud)
        error = 100*((baud2 - baud) / baud)
        if abs(error) < 1 and (int(baud) == baud or int(baud2) == baud2):
            print(abs(error), baud, baud2, n, x, k)

"""
for k in range(1, 10000):
    baud = pic_baud(k)
    baud2, n, x = closest_ft231_baud(baud)
    error = 100*((baud2 - baud) / baud)
    if abs(error) < 1:
        print(k, error, baud, baud2, n, x)
"""

    
