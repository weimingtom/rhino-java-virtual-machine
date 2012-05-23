#!/usr/bin/python3.1
import os
import sys
import pprint


def main():
  fd = open('log', 'r')
  lines = fd.readlines()
  fd.close()

  peak = {}
  a = {}
  f = {}

  calls = []

  tmu = 0
  for line in lines:
    line = line.strip()
    line = line.split(':')

    if line[0] != '##':
      continue

    logsys = line[1]

    # minfof("##:mi:alloc:%s:%u:%lx:%u\n", f, line, p, size);
    # minfof("##:mi:free:%u\n"..
    if logsys == 'ci':  
      if line[2] == 'return':
        calls.append(('return',))
      if line[2] == 'call':
        className = line[3]
        methodName = line[4]
        methodType = line[5]
        calls.append(('call', className, methodName, methodType))

    if logsys == 'mi':
      if line[2] == 'alloc':
        function = line[3]
        linenum = line[4]
        ptr = line[5]
        sz = int(line[6])

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

      if line[2] == 'free':
        ptr = line[3]
        __f, sz = a[ptr]
        a[ptr] = None
        f[__f] = f[__f] - sz

  sp = 0
  if 'ci' in sys.argv:
    for call in calls:
      if call[0] == 'return':
        sp = sp - 1
      if call[0] == 'call':
        txt = '%s:%s:%s' % (call[1], call[2], call[3])
        print('%s%s' % (' ' * sp * 2, txt))
        sp = sp + 1
      
  if 'mi' in sys.argv:
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
  return

 
main()