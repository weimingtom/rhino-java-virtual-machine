#include "rjvm.h"
#include "std.h"
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#define JVM_M_FREE      0
#define JVM_M_USED      (1 << 31)

#define JVM_M_ISUSED(fas) ((fas) & 0x80000000)
#define JVM_M_ISLAST(fs) ((fas) & 0x40000000)
#define JVM_M_SIZE(fas) ((fas) & ~0x80000000)

typedef struct _JVM_M_PT {
  uint32                fas;
} JVM_M_PT;

typedef struct _JVM_M_CH {
  struct _JVM_M_CH      *next;
  uint8                 mutex;
  uint32                size;
  uint32                free;
} JVM_M_CH;

typedef struct _JVM_M_MS {
  JVM_M_CH              *first;
} JVM_M_MS;

JVM_M_MS                g_jvm_m_ms;

void jvm_m_init() {
	g_jvm_m_ms.first = 0;
}

int jvm_m_give(void *ptr, uintptr size) {
  JVM_M_CH      *chunk;
  JVM_M_MS      *ms;
  JVM_M_PT      *pt;
  
  ms = &g_jvm_m_ms;

  debugf("give ptr:%x\n", ptr);
  
  chunk = (JVM_M_CH*)ptr;
  chunk->size = size - sizeof(JVM_M_CH);
  chunk->mutex = 0;
  // properly account for chunk and part headers
  chunk->free = size - sizeof(JVM_M_CH) - sizeof(JVM_M_PT);
  chunk->next = ms->first;
  ms->first = chunk;

  debugf("ms->first=%x\n", ms->first);

  pt = (JVM_M_PT*)((uintptr)chunk + sizeof(JVM_M_CH));
  pt->fas = chunk->free;
  return JVM_SUCCESS;
}

void *jvm_m_malloc(size) {
  JVM_M_MS              *ms;
  JVM_M_CH              *ch;
  JVM_M_PT              *pt, *_pt;
  int                   free;
  int                   rtotal;
  
  ms = &g_jvm_m_ms;
  _pt = 0;
  //debugf("malloc ms->first=%x\n", ms->first);
  for (ch = ms->first; ch != 0; ch = ch->next) {
    // check if block has enough free
    //debugf("check ch->free=%u >= %u\n", ch->free, size);
    if (ch->free >= size) {
      // lock the chunk
      jvm_MutexAquire(&ch->mutex);
      //debugf("mutex aquired\n");
      rtotal = -4;
      for (
              // grab first part for intialization
              pt = (JVM_M_PT*)((uintptr)ch + sizeof(JVM_M_CH));
              // make sure we have not gone past end of chunk
              ((uintptr)pt - (uintptr)ch) < ch->size;
              // get next part
              pt = (JVM_M_PT*)((uintptr)pt + JVM_M_SIZE(pt->fas) + sizeof(JVM_M_PT))
          )
      {
              //debugf("size:%x looking pt=%x pt-ch=%u ch->size:%u pt->fas:%u\n", size, pt, (uintptr)pt - (uintptr)ch, ch->size, pt->fas);
              // track consecutive free blocks
              if (JVM_M_ISUSED(pt->fas)) 
              {
                rtotal = (int)-sizeof(JVM_M_PT);
                _pt = 0;
			  } else {
                rtotal += JVM_M_SIZE(pt->fas) + sizeof(JVM_M_PT);
                // if 'pt' is zero then set it if not leave it
                if (!_pt)
					_pt = pt;
			  }
              debugf("ch:%llx rtotal:%i jvm_m_isfree:%u\n", ch, rtotal, JVM_M_ISUSED(pt->fas));
              // do we have enough?
              if (rtotal >= size) {
                //  do we have enough at the end to create a new part?
                if (rtotal - size > (2 * sizeof(JVM_M_PT))) {
                  // create two blocks one free one used                  
                  _pt->fas = JVM_M_USED | size;
                  pt = _pt;
                  _pt = (JVM_M_PT*)((uintptr)_pt + sizeof(JVM_M_PT) + size);
                  //debugf("1A %llx\n", _pt);
                  _pt->fas = JVM_M_FREE | (rtotal - size);
                  //debugf("2A\n");
                  //debugf("ret[split]: pt:%x pt->fas:%x _pt:%x _pt->fas:%x\n", pt, pt->fas, _pt, _pt->fas);
                  //ch->free -= sizeof(JVM_M_PT) + size;
                  jvm_MutexRelease(&ch->mutex);
                  return (void*)((uintptr)pt + sizeof(JVM_M_PT));
                } else {
			      //debugf("2\n");
                  // do not bother splitting use as whole
                  //debugf("ret[whole]\n");
                  _pt->fas = JVM_M_USED | rtotal;
                  //ch->free -= sizeof(JVM_M_PT) + rtotal;
                  //debugf("ret: pt[whole]:%x\n", pt);
                  jvm_MutexRelease(&ch->mutex);
                  return (void*)((uintptr)_pt + sizeof(JVM_M_PT));
                }
              }
      }
      jvm_MutexRelease(&ch->mutex);
    }
  }
  
  return 0;
}

void jvm_m_free(void *ptr) {
	JVM_M_PT		*_ptr;
	
	_ptr = (JVM_M_PT*)((uintptr)ptr - sizeof(JVM_M_PT));
	_ptr->fas = 0;
}

void jvm_free(void *p) {
  minfof("##:mi:free:%lx\n", p);
  free(p);
}

int jvm_strlen(const char *a) {
  return strlen(a);
}

void *jvm__malloc(uintptr size, const char *f, uint32 line) {
  void          *p;
  #ifdef MALLOCFORALLOC
  p = malloc(size);
  #else
  p = jvm_m_malloc(size);
  #endif
  minfof("##:mi:alloc:%s:%u:%lx:%u\n", f, line, p, size);
  return p;
}

/*
void jvm_printf(const char *fmt, ...) {
  va_list       args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}
*/

void jvm_exit(int code) {
  exit(code);
}

int jvm_strcmp(const char *a, const char *b)
{
  return strcmp(a, b);
}
