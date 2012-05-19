#!/usr/bin/python3.1
import os
import sys
import pprint


fd = open('log', 'r')
lines = fd.readlines()
fd.close()

peak = {}
a = {}
f = {}

tmu = 0
for line in lines:
  line = line.strip()
  line = line.split(':')
  method = line[1]

  if method == 'alloc':
    function = line[2]
    linenum = line[3]
    ptr = line[4]
    sz = int(line[5])

    if function not in f:
      f[function] = 0
    f[function] = f[function] + sz

    # actual peak
    _tmu = 0
    for k in f:
      _tmu = _tmu + f[k]
    if _tmu > tmu:
      tmu = _tmu
    # general peak
    if function not in peak:
      peak[function] = 0
    if f[function] > peak[function]:
      peak[function] = f[function]

    a[ptr] = (function, sz)
    
  if method == 'free':
    ptr = line[2]
    __f, sz = a[ptr]
    a[ptr] = None
    f[__f] = f[__f] - sz

print('----peak memory usage---')
tp = 0
for k in peak:
  print('%30s%10u' % (k, peak[k]))
  tp = tp + peak[k]
print('summation-peak: %s' % (tp))
print('actual-peak: %s' % (tmu))
print('----allocated memory on exit----')
for k in f:
  print('%30s%10u' % (k, f[k]))
 

 
