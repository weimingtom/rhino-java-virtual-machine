#!/usr/bin/python3.1
import os
import sys
import struct
import subprocess
import random
import time
import traceback
import sys

def stop():
	while True:
		pass

def main():
	p = subprocess.Popen(['./malloctestproxy'], stdin = subprocess.PIPE, stderr = subprocess.PIPE);

	alocs = {}
	addrs = []
	
	if sys.maxsize > 0xffffffff:
		ptrsize = 'Q'
	else:
		ptrsize = 'I'
		
	amtalloced = 0

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
			n = r.randint(5, 1024*512)
			# tell the proxy for testing the malloc alogrithm
			# that we wish to allocate 'n' number of bytes
			p.stdin.write(struct.pack('B', 0x00))
			p.stdin.write(struct.pack(ptrsize, n))
			p.stdin.flush()

			# read in the address given for the allocation of
			# specified size and convert it from bytes into
			# a 64-bit integer
			res = ''
			while len(res) == 0:
				res = p.stderr.read(struct.calcsize(ptrsize))
			addr = struct.unpack_from(ptrsize, res)[0]

			# this can happen and i want to test against it
			# because things have to be done a certain way
			# and also we are testing the logic for when we
			# bump against the ceiling and allocate that very
			# last block..
			if addr == 0:
				print('[!] got out of memory code or error')
				print('[!] allocated memory is %s' % (amtalloced))
				continue

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
			#print('PY-ALLOC', hex(addr), hex(n))
			amtalloced = amtalloced + n
		else:
			# free operation (free a previous allocation)	
			k = addrs[r.randint(0, len(addrs) - 1)]
			# just a simple sanity check
			if k not in alocs:
				print('addr not in allocs (if this even possible, LOL)', addr);
				stop()
			# remove from our data
			amtalloced = amtalloced - alocs[k]
			addrs.remove(k)
			del alocs[k]
			# tell the test proxy to perform the free operation
			p.stdin.write(struct.pack('B', 0x01))
			p.stdin.write(struct.pack(ptrsize, k))
			p.stdin.flush()
			#print('PY-FREE', hex(k))
		print('allocated-memory:%s' % (amtalloced))
    
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
