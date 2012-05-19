#ifndef RJVM_MS_H
#define RJVM_MS_H
#include "rjvm.h"

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