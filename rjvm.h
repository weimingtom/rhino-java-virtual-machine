#ifndef RJVM_RJVM_H
#define RJVM_RJVM_H
#include "port.h"
#include "std.h"

#define DEBUG_INFO
#ifdef DEBUG_INFO
#define debugf jvm_printf("[%s:%u] ", __FUNCTION__, __LINE__); jvm_printf
#else
#define debugf //
#endif

#define ERROR_INFO
#ifdef ERROR_INFO
#define errorf jvm_printf
#else
#define errorf //
#endif

#define CALL_INFO
#ifdef CALL_INFO
#define callinfof jvm_printf
#else
#define callinfof //
#endif

#define MALLOC_INFO
#ifdef MALLOC_INFO
#define minfof jvm_printf
#else
#define minfof //
#endif


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

#define JVM_SUCCESS                      0
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
#define JVM_ERROR_MISSINGFIELD          -16
#define JVM_ERROR_INVALIDARG            -17

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

#define JVM_ATYPE_BYTE          8
#define JVM_ATYPE_CHAR          5
#define JVM_ATYPE_INT           10
#define JVM_ATYPE_LONG          11
#define JVM_ATYPE_FLOAT         6
#define JVM_ATYPE_DOUBLE        7
#define JVM_ATYPE_BOOL          4
#define JVM_ATYPE_SHORT         9

// For garbage collection I needed this to easily
// determine what type of an object I am dealing
// with.
#define JVM_OBJTYPE_PARRAY      1
#define JVM_OBJTYPE_OBJECT      2
#define JVM_OBJTYPE_OARRAY      3

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

typedef struct _JVMConstPoolLong {
  JVMConstPoolItem      hdr;
  uint32                high;
  uint32                low;
} JVMConstPoolLong;

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

/// specifies this function has not java implementation
/// also the class must have the JVM_CLASS_NATIVE set
/// and it must be set in our jvm since it is not supported
/// by the java specification
#define JVM_ACC_NATIVE          0x00000100
#define JVM_ACC_PUBLIC          0x00000001
#define JVM_ACC_PRIVATE         0x00000002
#define JVM_ACC_PROTECTED       0x00000004
#define JVM_ACC_STATIC          0x00000008
#define JVM_ACC_FINAL           0x00000010
#define JVM_ACC_VOLATILE        0x00000040
#define JVM_ACC_TRANSIENT       0x00000080
#define JVM_ACC_SYNTHETIC       0x00001000
#define JVM_ACC_ENUM            0x00004000

typedef struct _JVMMethod {
  uint16                accessFlags;
  uint16                nameIndex;
  uint16                descIndex;
  uint16                attrCount;
  JVMAttribute          *attrs;
  JVMCodeAttribute      *code;
} JVMMethod;

/// specifies the class has native methods
#define JVM_CLASS_NATIVE                1

struct _JVM;
struct _JVMBundle;
struct _JVMClass;
typedef int (*PFNativeHandler)(struct _JVM *jvm, struct _JVMBundle *bundle, struct _JVMClass *jclass,
                               uint8 *method8, uint8 *type8, JVMLocal *locals,
                               int localCnt, JVMLocal *result);
struct _JVMObjectField;

typedef struct _JVMClass {
  uint16                        poolCnt;
  JVMConstPoolItem              **pool;
  uint16                        accessFlags;
  uint16                        thisClass;
  uint16                        superClass;
  uint16                        ifaceCnt;
  uint16                        *interfaces;
  uint16                        fieldCnt;
  JVMClassField                 *fields;
  uint16                        methodCnt;
  JVMMethod                     *methods;
  uint16                        attrCnt;
  JVMAttribute                  *attrs;
  // set to JVM_CLASS_NATIVE then any native method
  // call will result in a call to the handler
  uint32                        flags;
  // native handler function pointer
  PFNativeHandler               nhand;
  // static fields
  uint16                        sfieldCnt;
  struct _JVMObjectField        *sfields;
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
  // how many instances are on stack and locals
  uint16                        stackCnt;
  // garbage collector mark
  uint16                        cmark;
  // the object absolute type
  uint8                         type;
  // monitor mutex
  uint8                         mutex;
} JVMObject;

typedef struct _JVM {
  // all objects instanced on heap
  JVMObject             *objects;
  // last garbage collector mark
  uint16                cmark;
  // global lock to object chain
  uint8                 mutex;
  // the bundle of loaded classes
  JVMBundle             *bundle;
} JVM;

// java stores all integers in big-endian
#define LENDIAN
#ifdef LENDIAN
#define noths(x) ((x) >> 8 | ((x) & 0xff) << 8)
#define nothl(x) ((x) >> 24 | ((x) & 0xff0000) >> 8 | ((x) & 0xff00) << 8 | (x) << 24)
#endif
#ifdef BENDIAN
#define noths(x) x
#define nothl(x) x
#endif

JVMClass* jvm_LoadClass(struct _JVMMemoryStream *m);
JVMClass* jvm_FindClassInBundle(JVMBundle *bundle, const char *className);
JVMMethod* jvm_FindMethodInClass(JVMClass *jclass, const char *methodName, const char *methodType);
int jvm_IsMethodReturnTypeVoid(const char *typestr);
int jvm_GetMethodTypeArgumentCount(const char *typestr);
void jvm_ScrubLocals(JVMLocal *locals, uint8 maxLocals);
void jvm_ScrubStack(JVMStack *stack);
int jvm_IsInstanceOf(JVMBundle *bundle, JVMObject *jobject, uint8 *className);
int jvm_CreateObject(JVM *jvm, JVMBundle *bundle, const char *className, JVMObject **out);
uint8* jvm_ReadWholeFile(const char *path, uint32 *size);
void jvm_AddClassToBundle(JVMBundle *jbundle, JVMClass *jclass);
uint8* jvm_GetClassNameFromClass(JVMClass *c);
int jvm_Collect(JVM *jvm);
int jvm_PutField(JVMBundle *bundle, JVMObject *jobject, uint8 *fieldName, uintptr data, uint32 flags);
int jvm_GetField(JVMObject *jobject, uint8 *fieldName, JVMLocal *result);
int jvm_CreateString(JVM *jvm, JVMBundle *bundle, uint8 *string, uint16 szlen, JVMObject **out);
void jvm_MutexAquire(uint8 *mutex);
void jvm_MutexRelease(uint8 *mutex);
#endif