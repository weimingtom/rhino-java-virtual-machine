#ifndef RHINO_JVM_EXEC_H
#define RHINO_JVM_EXEC_H
#include "rjvm.h"

int jvm_CreateObjectArray(JVM *jvm, JVMBundle *bundle, uint8 *className, uint32 size, JVMObject **_object);
int jvm_CreatePrimArray(JVM *jvm, JVMBundle *bundle, uint8 type, uint32 cnt, JVMObject **jobject);

int jvm_ExecuteObjectMethod(JVM *jvm, JVMBundle *bundle, JVMClass *jclass,
                         const char *methodName, const char *methodType,
                         JVMLocal *_locals, uint8 localCnt, JVMLocal *_result);
#endif