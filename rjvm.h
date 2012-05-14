#ifndef RJVM_RJVM_H
#define RJVM_RJVM_H
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

struct _JVMMemoryStream;

#define TAG_METHODREF                   10
#define TAG_CLASSINFO                   7
#define TAG_NAMEANDTYPE                 12
#define TAG_UTF8                        1
#define TAG_FIELDREF                    9
#define TAG_STRING                      8
#define TAG_INTEGER                     3
#define TAG_FLOAT                       4
#define TAG_LONG                        5
#define TAG_DOUBLE                      6

typedef unsigned long long int  uint64;
typedef signed long long int    int64;
typedef unsigned int            uint32;
typedef unsigned short          uint16;
typedef unsigned char           uint8;
typedef signed int              int32;
typedef signed short            int16;
typedef signed char             int8;
typedef uint64                  uintptr;
typedef int64                   intptr;

#define JVM_SUCCESS                      1
#define JVM_ERROR_METHODNOTFOUND        -1
#define JVM_ERROR_OUTOFMEMORY           -2
#define JVM_ERROR_UNKNOWNOPCODE         -3
#define JVM_ERROR_CLASSNOTFOUND         -4
#define JVM_ERROR_ARRAYOUTOFBOUNDS      -5
#define JVM_ERROR_NOTOBJREF             -6
#define JVM_ERROR_SUPERMISSING          -7
#define JVM_ERROR_NULLOBJREF            -8
#define JVM_ERROR_NOCODE                -9
#define JVM_ERROR_EXCEPTION             -10
#define JVM_ERROR_BADCAST               -11
#define JVM_ERROR_NOTOBJORARRAY         -12
#define JVM_ERROR_WASNOTINSTANCEOF      -13
#define JVM_ERROR_WASPRIMITIVEARRAY     -14
#define JVM_ERROR_FIELDTYPEDIFFERS      -15

/*
  I have yet to use the other flags. Currently,
  I am using JVM_STACK_ISOBJECTREF soley, but
  the others are here incase I realized in my
  design I need to track the exact types better.

  NATOBJECT is short for native object
  Essentially, all method call are caught and
  instead handed to a special function
*/
#define JVM_STACK_ISOBJECTREF   0x00000001
#define JVM_STACK_ISARRAYREF    0x00000002
#define JVM_STACK_ISBYTE        0x00000080
#define JVM_STACK_ISCHAR        0x00000050
#define JVM_STACK_ISDOUBLE      0x00000070
#define JVM_STACK_ISFLOAT       0x00000060
#define JVM_STACK_ISINT         0x000000a0
#define JVM_STACK_ISLONG        0x000000b0
#define JVM_STACK_ISSHORT       0x00000090
#define JVM_STACK_ISBOOL        0x00000040
#define JVM_STACK_ISNULL        0x000000f0
/// special 
#define JVM_STACK_STRING        0x00000004

#define JVM_ATYPE_BYTE          8
#define JVM_ATYPE_CHAR          5
#define JVM_ATYPE_INT           10
#define JVM_ATYPE_LONG          11
#define JVM_ATYPE_FLOAT         6
#define JVM_ATYPE_DOUBLE        7
#define JVM_ATYPE_BOOL          4
#define JVM_ATYPE_SHORT         9

#define debugf printf("[%s:%u] ", __FUNCTION__, __LINE__); printf

typedef struct _JVMStack {
  uintptr               *data;
  uint32                *flags;
  uint32                pos;
  uint32                max;
} JVMStack;

typedef struct _JVMLocal {
  uintptr               data;
  uint32                flags;
} JVMLocal;

typedef struct _JVMConstPoolItem {
  uint8                 type;
} JVMConstPoolItem;

typedef struct _JVMConstPoolMethodRef {
  JVMConstPoolItem      hdr;
  uint32                nameIndex;              // name index
  uint32                descIndex;              // descriptor index
} JVMConstPoolMethodRef;

typedef struct _JVMConstPoolClassInfo {
  JVMConstPoolItem      hdr;
  uint32                nameIndex;
} JVMConstPoolClassInfo;

typedef struct _JVMConstPoolString {
  JVMConstPoolItem      hdr;
  uint16                stringIndex;
} JVMConstPoolString;

typedef struct _JVMConstPoolInteger {
  JVMConstPoolItem      hdr;
  uint32                value;
} JVMConstPoolInteger;

typedef struct _JVMConstPoolUtf8 {
  JVMConstPoolItem      hdr;
  uint16                size;
  uint8                 *string;
} JVMConstPoolUtf8;

typedef struct _JVMConstPoolNameAndType {
  JVMConstPoolItem      hdr;
  uint32                nameIndex;
  uint32                descIndex;
} JVMConstPoolNameAndType;

typedef struct _JVMConstPoolFieldRef {
  JVMConstPoolItem      hdr;
  uint32                classIndex;
  uint32                nameAndTypeIndex;
} JVMConstPoolFieldRef;

typedef struct _JVMClassField {
  uint16                accessFlags;
  uint16                nameIndex;
  uint16                descIndex;
  uint16                attrCount;
} JVMClassField;

typedef struct _JVMAttribute {
  uint16                nameIndex;
  uint32                length;
  uint8                 *info;
} JVMAttribute;

typedef struct _JVMExceptionTable {
  uint16                pcStart;
  uint16                pcEnd;
  uint16                pcHandler;
  uint16                catchType;
} JVMExceptionTable;

typedef struct _JVMCodeAttribute {
  uint16                attrNameIndex;
  uint32                attrLength;
  uint16                maxStack;
  uint16                maxLocals;
  uint32                codeLength;
  uint8                 *code;
  uint16                eTableCount;
  JVMExceptionTable     *eTable;
  uint16                attrsCount;
  JVMAttribute          *attrs;
} JVMCodeAttribute;

typedef struct _JVMMethod {
  uint16                accessFlags;
  uint16                nameIndex;
  uint16                descIndex;
  uint16                attrCount;
  JVMAttribute          *attrs;
  JVMCodeAttribute      *code;
} JVMMethod;

typedef struct _JVMClass {
  uint16                poolCnt;
  JVMConstPoolItem      **pool;
  uint16                accessFlags;
  uint16                thisClass;
  uint16                superClass;
  uint16                ifaceCnt;
  uint16                *interfaces;
  uint16                fieldCnt;
  JVMClassField         *fields;
  uint16                methodCnt;
  JVMMethod             *methods;
  uint16                attrCnt;
  JVMAttribute          *attrs;
} JVMClass;

typedef struct _JVMBundleClass {
  struct _JVMBundleClass        *next;
  JVMClass                      *jclass;
  const char                    *nameSpace;
} JVMBundleClass;

typedef struct _JVMBundle {
  JVMBundleClass                *first;
} JVMBundle;

typedef struct _JVMObjectField {
    uint8                       *name;
    JVMClass                    *jclass;
    uintptr                     value;
    uint32                      flags;
    uint32                      aflags;
} JVMObjectField;

typedef struct _JVMObject {
  struct _JVMObject             *next;
  // used as max count by primitive
  // arrays, but is the type for obj
  // reference arrays
  JVMClass                      *class;
  union {
  // used by arrays
  uint64                        *fields;
  // used by non-array objects
  JVMObjectField                *_fields;
  };
  uint16                        fieldCnt;
  // never multi-purpose, needed
  struct _JVMObject             *fwdref;
  struct _JVMObject             *bckref;
  /// need to be removed.. asap
  void                          *refs;
  //
  int32                         stackCnt;
} JVMObject;

typedef struct _JVM {
  JVMObject             *objects;
} JVM;

JVMClass* jvm_LoadClass(struct _JVMMemoryStream *m);
JVMClass* jvm_FindClassInBundle(JVMBundle *bundle, const char *className);
JVMMethod* jvm_FindMethodInClass(JVMClass *jclass, const char *methodName, const char *methodType);
int jvm_IsMethodReturnTypeVoid(const char *typestr);
int jvm_GetMethodTypeArgumentCount(const char *typestr);
void jvm_ScrubLocals(JVMLocal *locals);
void jvm_ScrubStack(JVMStack *stack);
int jvm_IsInstanceOf(JVMBundle *bundle, JVMObject *jobject, uint8 *className);
int jvm_CreateObject(JVM *jvm, JVMBundle *bundle, const char *className, JVMObject **out);
uint8* jvm_ReadWholeFile(const char *path, uint32 *size);
void jvm_AddClassToBundle(JVMBundle *jbundle, JVMClass *jclass);
uint8* jvm_GetClassNameFromClass(JVMClass *c);
#endif