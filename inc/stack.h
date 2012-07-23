#ifndef RJVM_STACK_H
#define RJVM_STACK_H
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "rjvm.h"

void jvm_StackDiscardTop(JVMStack *stack);
void jvm_StackPop(JVMStack *stack, JVMLocal *local);
void jvm_StackPeek(JVMStack *stack, JVMLocal *local);
void jvm_StackPush(JVMStack *stack, int64 value, uint32 flags);
void jvm_StackInit(JVMStack *stack, uint32 max);
void jvm_StackFree(JVMStack *stack);
int jvm_StackMore(JVMStack *stack);
void jvm_DebugStack(JVMStack *stack);
#endif
