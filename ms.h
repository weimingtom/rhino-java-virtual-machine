#ifndef RJVM_MS_H
#define RJVM_MS_H
#include "rjvm.h"

// java stores all integers in big-endian
#define LENDIAN
#ifdef LENDIAN
#define noths(x) ((x) >> 8 | ((x) & 0xff) << 8)
#define nothl(x) ((x) >> 24 | ((x) & 0xff0000) >> 8 | ((x) & 0xff00) << 8 | (x) << 24)
#endif
#ifdef BENDIAN
#define noths(x) x
#define nothl(x) x
#endif

typedef struct _JVMMemoryStream {
  uint8                 *data;
  uint32                pos;
  uint32                size;
} JVMMemoryStream;

void msWrap(JVMMemoryStream *m, void *buf, uint32 size);
uint32 msRead32(JVMMemoryStream *m);
uint16 msRead16(JVMMemoryStream *m); 
uint8 msRead8(JVMMemoryStream *m);
uint8* msRead(JVMMemoryStream *m, uint32 sz, uint8 *buf);
#endif