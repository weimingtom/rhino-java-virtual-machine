#ifndef RJVM_STD_H
#define RJVM_STD_H
#include "port.h"
#include <stdio.h>

void jvm_free(void *p);
#define jvm_malloc(s) jvm__malloc((s), __FUNCTION__, __LINE__);
void *jvm__malloc(uintptr size, const char *function, uint32 line);
//void jvm_printf(const char *fmt, ...);
#define jvm_printf printf
void jvm_exit(int result);
int jvm_strcmp(const char *a, const char *b);
int jvm_strlen(const char *a);
void jvm_PrintMemoryDiag();
#endif