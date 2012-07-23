#ifndef RJVM_RMALLOC_H
#define RJVM_RMALLOC_H
#include "rjvm.h"

#define JVM_M_FREE      0
#define JVM_M_USED      0x80000000

#define JVM_M_ISUSED(fas) ((fas) & 0x80000000)
#define JVM_M_ISLAST(fs) ((fas) & 0x40000000)
#define JVM_M_SIZE(fas) ((fas) & ~0x80000000)

typedef struct _JVM_M_PT {
  uint32                fas;
} JVM_M_PT;

typedef struct _JVM_M_CH {
  struct _JVM_M_CH      *next;
  JVM_MUTEX             mutex;
  uint32                size;
  uint32                free;
} JVM_M_CH;

typedef struct _JVM_M_MS {
  JVM_M_CH              *first;
} JVM_M_MS;

void *jvm_m_malloc(int32 size);
void jvm_m_free(void *ptr);
void jvm_m_init();
#endif
