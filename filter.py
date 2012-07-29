#!/usr/bin/python3.1
import sys

stdin = sys.stdin

while True:
  line = stdin.buffer.readline().strip().decode('utf-8', 'ignore')
  idtag = line[1:line.find(']')]
  idtag = idtag.split(':')
  try:
    lineno = int(idtag.pop(-1))
  except ValueError as e:
    lineno = -1
  method = ':'.join(idtag)

  #if (method == 'jvm_ExecuteObjectMethod' and lineno == 228):
  print(line)
  
  
print('[filter done]')
