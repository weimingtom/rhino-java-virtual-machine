#include "ms.h"

void msWrap(JVMMemoryStream *m, void *buf, uint32 size) {
  m->pos = 0;
  m->data = buf;
  m->size = size;
}

uint32 msRead32(JVMMemoryStream *m) {
  uint32                v;
  v = ((uint32*)&m->data[m->pos])[0];
  m->pos += 4;
  return nothl(v);
}
uint16 msRead16(JVMMemoryStream *m) {
  uint16                v;
  v = ((uint16*)&m->data[m->pos])[0];
  m->pos += 2;
  return noths(v);
}

uint8 msRead8(JVMMemoryStream *m) {
  uint8                 v;
  v = ((uint8*)&m->data[m->pos])[0];
  m->pos += 1;
  return v;
}

uint8* msRead(JVMMemoryStream *m, uint32 sz, uint8 *buf) {
  uint32        x;
  uint32        p;

  p = m->pos;
  for (x = 0; x < sz; ++x)
    buf[x] = m->data[x + p];

  m->pos += sz;
  return buf;
}
