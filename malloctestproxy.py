#!/usr/bin/python3.1
import os
import sys
import struct
import subprocess
import random
import time
import traceback

def stop():
	while True:
		pass

def main():
	p = subprocess.Popen(['./malloctestproxy'], stdin = subprocess.PIPE, stderr = subprocess.PIPE);

	alocs = {}
	addrs = []

	r = random.Random(time.time())
	while True:
		# determine if we want to allocate or free
		o = r.randint(0, 1)
		# if we have never allocated we can not free
		# so force to allocate operation this cycle
		if len(addrs) == 0:
			o = 0

		# allocate operation
		if o == 0:
			n = r.randint(5, 128)
			# tell the proxy for testing the malloc alogrithm
			# that we wish to allocate 'n' number of bytes
			p.stdin.write(struct.pack('B', 0x00))
			p.stdin.write(struct.pack('Q', n))
			p.stdin.flush()

			# read in the address given for the allocation of
			# specified size and convert it from bytes into
			# a 64-bit integer
			res = p.stderr.read(8)
			print('len(res)', len(res))
			addr = struct.unpack_from('II', res)
			addr = addr[0] | addr[1] << 32

			# make sure not only did we not get an already
			# used address, but also check this address 
			# does not fall inside a currently alloced block
			for a in alocs:
				s = alocs[a]
				if (addr >= a) and (addr < (a + s)):
				  print('[!] addr inside prior allocation!')
				  print('[!] prior:%x at length %x (this is %x)' % (a, s, addr))
				  print('[!] test failed')
				  stop()
			# store a record of this allocation
			addrs.append(addr)
			alocs[addr] = n
			print('PY-ALLOC', hex(addr), hex(n))
			if addr == 0:
				print('[!] addr returned from malloc zero')
				print('[!] test _maybe_ failed (anyone can run something out of memory)')
				stop()
		else:
			# free operation (free a previous allocation)	
			k = addrs[r.randint(0, len(addrs) - 1)]
			# just a simple sanity check
			if k not in alocs:
				print('addr not in allocs', addr);
				stop()
			# remove from our data
			addrs.remove(k)
			del alocs[k]
			# tell the test proxy to perform the free operation
			p.stdin.write(struct.pack('B', 0x01))
			p.stdin.write(struct.pack('Q', k))
			p.stdin.flush()
			print('PY-FREE', hex(k))
    
try:
	main()
except Exception:
	exc_type, exc_value, exc_traceback = sys.exc_info()
	print('*** print_tb:')
	traceback.print_tb(exc_traceback, limit=1, file=sys.stdout)
	print('*** print_exception:')
	traceback.print_exception(exc_type, exc_value, exc_traceback, limit=2, file=sys.stdout)
	exit(3)

#log.write(sys.stdin.read())
#log.flush()
