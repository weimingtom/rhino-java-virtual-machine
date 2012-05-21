#include "rjvm.h"
#include "std.h"
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

void jvm_free(void *p) {
  minfof("##:free:%lx\n", p);
  free(p);
}

int jvm_strlen(const char *a) {
  return strlen(a);
}

void *jvm__malloc(uintptr size, const char *f, uint32 line) {
  void          *p;
  p = malloc(size);
  minfof("##:alloc:%s:%u:%lx:%u\n", f, line, p, size);
  return p;
}

void jvm_PrintMemoryDiag() {
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