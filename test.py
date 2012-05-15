#!/usr/bin/python3.1
import os
import sys
import subprocess

JAVAC = 'javac'
JAVA = 'java'


nodes = os.listdir('./tests')

pd = subprocess.Popen("make", stdout= subprocess.PIPE, stderr = subprocess.PIPE)
err = pd.stderr.read()
if len(err) > 0:
  print('error during make')
  exit(-5)

for node in nodes:
  base = node[0:node.find('.')]
  ext = node[node.find('.')+1:]

  if ext != 'java':
    continue

  # get modified timestamps
  jmt = 0
  cmt = 0
  if os.path.exists('./tests/%s.java' % base):
    stat = os.stat('./tests/%s.java' % base)
    jmt = stat.st_mtime
  if os.path.exists('./tests/%s.class' % base):
    stat = os.stat('./tests/%s.class' % base)
    cmt = stat.st_mtime
  print(jmt, cmt)
  # do we need to compile?
  if jmt > cmt:
    print('[%s] compile..' % base, end='')
    pd = subprocess.Popen(['javac', './tests/%s.java' % base], stderr = subprocess.PIPE)
    d = pd.stderr.read()
    if len(d) > 0:
      print('ERROR')
      print(bytes.decode(d, 'utf8'))
      continue
    print('')
  # run alpha
  print('[%s] alpha..' % base, end = '')
  args = ['./rhino', './tests/%s.class' % base, 'tests/%s' % base]
  pd = subprocess.Popen(args, stdout = subprocess.PIPE)
  d = pd.stdout.read()
  p = d.find(b'\ndone! result.data:')
  if p == -1:
    print(bytes.decode(d, 'utf8'))
    print('[FAILED-INCOMPLETE]')
    exit(-1)
  d = d[p:]
  d = d[19:]
  d = d[:d.find(b' ')]
  alphaResult = int(d)
  print('[%s]' % alphaResult)
  
  # run beta
  print('[%s] beta..' % base, end='')
  pd = subprocess.Popen(['java', 'tests.%s' % base], stdout = subprocess.PIPE)
  d = pd.stdout.read()
  betaResult = int(d)

  print('[%s]' % betaResult)


  