#ifndef RHINO_JVM_EXEC_H
#define RHINO_JVM_EXEC_H
#include "rjvm.h"

int jvm_ExecuteObjectMethod(JVM *jvm, JVMBundle *bundle, JVMClass *jclass,
                         const char *methodName, const char *methodType,
                         JVMLocal *_locals, uint8 localCnt, JVMLocal *_result);
#endif