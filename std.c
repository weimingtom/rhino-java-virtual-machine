#include "rjvm.h"
#include "std.h"
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

uint8* jvm_ReadWholeFile(const char *path, uint32 *size) {
  uint8         *buf;
  FILE          *fp;
  
  fp = fopen(path, "rb");
  if (!fp) {
    debugf("error could not open file %s", path);
    jvm_exit(-4);
  }
  fseek(fp, 0, 2);
  *size = ftell(fp);
  fseek(fp, 0, 0);
  buf = (uint8*)jvm_malloc(*size);
  fread(buf, 1, *size, fp);
  fclose(fp);
  
  return buf;
}

void jvm_free(void *p) {
  #ifndef INTERNALMALLOC
  free(p);
  #else
  jvm_m_free(p);
  #endif
  minfof("##:mi:free:%lx\n", p);
}

int jvm_strlen(const char *a) {
  return strlen(a);
}

void *jvm__malloc(uintptr size, const char *f, uint32 line) {
  void          *p;
  #ifndef INTERNALMALLOC 
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
