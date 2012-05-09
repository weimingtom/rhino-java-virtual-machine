#include <stdio.h>
#include <malloc.h>
#include <string.h>

typedef unsigned long long int  uint64;
typedef signed long long int    int64;
typedef unsigned int            uint32;
typedef unsigned short          uint16;
typedef unsigned char           uint8;
typedef signed int              int32;
typedef signed short            int16;
typedef signed char             int8;

#define JVM_SUCCESS                      1
#define JVM_ERROR_METHODNOTFOUND        -1
#define JVM_ERROR_OUTOFMEMORY           -2
#define JVM_ERROR_UNKNOWNOPCODE         -3
#define JVM_ERROR_CLASSNOTFOUND         -4

/*
  I have yet to use the other flags. Currently,
  I am using JVM_STACK_ISOBJECTREF soley, but
  the others are here incase I realized in my
  design I need to track the exact types better.
*/
#define JVM_STACK_ISOBJECTREF   0x00000001
#define JVM_STACK_ISBYTE        0x00000010
#define JVM_STACK_ISCHAR        0x00000020
#define JVM_STACK_ISDOUBLE      0x00000030
#define JVM_STACK_ISFLOAT       0x00000040
#define JVM_STACK_ISINT         0x00000050
#define JVM_STACK_ISLONG        0x00000060
#define JVM_STACK_ISSHORT       0x00000070
#define JVM_STACK_ISBOOL        0x00000080
#define JVM_STACK_ISARRAY       0x00000090

typedef struct _JVMStack {
  uint64                *data;
  uint32                *flags;
  uint32                pos;
  uint32                max;
} JVMStack;

typedef struct _JVMLocal {
  uint64                data;
  uint32                flags;
} JVMLocal;

void jvm_StackInit(JVMStack *stack, uint32 max) {
  stack->max = max;
  stack->pos = 0;
  stack->data = (uint64*)malloc(sizeof(uint64) * max);
  stack->flags = (uint32*)malloc(sizeof(uint32) * max);
}

int jvm_StackMore(JVMStack *stack) {
  return stack->pos;
}

void jvm_DebugStack(JVMStack *stack) {
  int           x;

  printf("DBGSTACK stack->pos:%u\n", stack->pos);
  for (x = stack->pos - 1; x > -1; --x)
  {
    printf("STACK[%u]: %u:%u\n", x, stack->data[x], stack->flags[x]);
  }
}

void jvm_StackDiscardTop(JVMStack *stack) {
  stack->pos--;
}

void jvm_StackPush(JVMStack *stack, uint64 value, uint32 flags) {
  stack->flags[stack->pos] = flags;
  stack->data[stack->pos] = value;
  stack->pos++;
  printf("stack push pos:%u\n", stack->pos);
  printf("value:%u flags:%u\n", value, flags);
  jvm_DebugStack(stack);
}

void jvm_StackPop(JVMStack *stack, JVMLocal *local) {
  --stack->pos;
  local->flags = stack->flags[stack->pos];
  local->data = stack->data[stack->pos];
  printf("stack-pop: pos:%u\n", stack->pos);
}

typedef struct _JVMConstPoolItem {
  uint8			type;
} JVMConstPoolItem;

typedef struct _JVMConstPoolMethodRef {
  JVMConstPoolItem	hdr;
  uint32		nameIndex;		// name index
  uint32		descIndex;		// descriptor index
} JVMConstPoolMethodRef;

typedef struct _JVMConstPoolClassInfo {
  JVMConstPoolItem	hdr;
  uint32		nameIndex;
} JVMConstPoolClassInfo;

typedef struct _JVMConstPoolUtf8 {
  JVMConstPoolItem	hdr;
  uint16		size;
  uint8			*string;
} JVMConstPoolUtf8;

typedef struct _JVMConstPoolNameAndType {
  JVMConstPoolItem	hdr;
  uint32		nameIndex;
  uint32		descIndex;
} JVMConstPoolNameAndType;

typedef struct _JVMConstPoolFieldRef {
  JVMConstPoolItem	hdr;
  uint32		classIndex;
  uint32		nameAndTypeIndex;
} JVMConstPoolFieldRef;

typedef struct _JVMClassField {
  uint16		accessFlags;
  uint16		nameIndex;
  uint16		descIndex;
  uint16		attrCount;
} JVMClassField;

typedef struct _JVMAttribute {
  uint16		nameIndex;
  uint32		length;
  uint8			*info;
} JVMAttribute;

typedef struct _JVMMethod {
  uint16		accessFlags;
  uint16		nameIndex;
  uint16		descIndex;
  uint16		attrCount;
  JVMAttribute		*attrs;
} JVMMethod;

typedef struct _JVMClass {
  uint16		poolCnt;
  JVMConstPoolItem	**pool;
  uint16		accessFlags;
  uint16		thisClass;
  uint16		superClass;
  uint16		ifaceCnt;
  uint16		*interfaces;
  uint16		fieldCnt;
  JVMClassField		*fields;
  uint16		methodCnt;
  JVMMethod		*methods;
  uint16		attrCnt;
  JVMAttribute		*attrs;
} JVMClass;

typedef struct _JVMBundleClass {
  struct _JVMBundleClass	*next;
  JVMClass			*jclass;
  const char                    *nameSpace;
} JVMBundleClass;

typedef struct _JVMBundle {
  JVMBundleClass		*first;
} JVMBundle;

typedef struct _JVMMemoryStream {
  uint8			*data;
  uint32		pos;
  uint32		size;
} JVMMemoryStream;

typedef struct _JVMObject {
  struct _JVMObject             *next;
  JVMClass                      *class;
  uint64                        *fields;
  struct _JVMObject             *refs;
  int32                         stackCnt;
  
} JVMObject;

typedef struct _JVM {
  JVMObject             *objects;
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

void msWrap(JVMMemoryStream *m, void *buf, uint32 size) {
  m->pos = 0;
  m->data = buf;
  m->size = size;
}

uint32 msRead32(JVMMemoryStream *m) {
  uint32		v;
  v = ((uint32*)&m->data[m->pos])[0];
  m->pos += 4;
  return nothl(v);
}
uint16 msRead16(JVMMemoryStream *m) {
  uint16		v;
  v = ((uint16*)&m->data[m->pos])[0];
  m->pos += 2;
  return noths(v);
}

uint8 msRead8(JVMMemoryStream *m) {
  uint8			v;
  v = ((uint8*)&m->data[m->pos])[0];
  m->pos += 1;
  return v;
}

uint8* msRead(JVMMemoryStream *m, uint32 sz, uint8 *buf) {
  uint32	x;
  uint32	p;
  
  p = m->pos;
  for (x = 0; x < sz; ++x)
    buf[x] = m->data[x + p];
  
  m->pos += sz;
  return buf;
}

#define TAG_METHODREF			10
#define TAG_CLASSINFO			7
#define TAG_NAMEANDTYPE			12
#define TAG_UTF8			1
#define TAG_FIELDREF			9

JVMClass* jvm_LoadClass(JVMMemoryStream *m) {
  uint16			vmin;
  uint16			vmaj;
  uint32			magic;
  uint16			cpoolcnt;
  uint8				tag;
  JVMConstPoolItem		**pool;
  JVMConstPoolItem		*pi;
  int				x, y;
  JVMConstPoolMethodRef		*pimr;
  JVMConstPoolClassInfo		*pici;
  JVMConstPoolUtf8		*piu8;
  JVMConstPoolNameAndType	*pint;
  JVMConstPoolFieldRef		*pifr;
  JVMClass			*class;
  
  magic = msRead32(m);
  vmin = msRead16(m);
  vmaj = msRead16(m);
  
  class = (JVMClass*)malloc(sizeof(JVMClass));
  /*
    ==================================
    LOAD CONSTANT POOL TABLE
    ==================================
  */
  cpoolcnt = msRead16(m); 
  pool = (JVMConstPoolItem**)malloc(sizeof(JVMConstPoolItem*) * cpoolcnt);
  class->poolCnt = cpoolcnt;
  class->pool = pool;
  fprintf(stderr, "cpoolcnt:%u", cpoolcnt);
  for (x = 0; x < cpoolcnt - 1; ++x) {
    tag = msRead8(m);
    switch (tag) {
      case TAG_METHODREF:
	pimr = (JVMConstPoolMethodRef*)malloc(sizeof(JVMConstPoolMethodRef));
	pool[x] = (JVMConstPoolItem*)pimr;
	pimr->nameIndex = msRead16(m);
	pimr->descIndex = msRead16(m);
	pimr->hdr.type = TAG_METHODREF;
	fprintf(stderr, "nameIndex:%u descIndex:%u\n", pimr->nameIndex, pimr->descIndex);
	break;
      case TAG_CLASSINFO:
	pici = (JVMConstPoolClassInfo*)malloc(sizeof(JVMConstPoolClassInfo));
	pool[x] = (JVMConstPoolItem*)pici;
	pici->nameIndex = msRead16(m);
	pici->hdr.type = TAG_CLASSINFO;
	break;
      case TAG_UTF8:
	piu8 = (JVMConstPoolUtf8*)malloc(sizeof(JVMConstPoolUtf8));
	pool[x] = (JVMConstPoolItem*)piu8;
	piu8->size = msRead16(m);
	piu8->string = (uint8*)malloc(piu8->size + 1);
	msRead(m, piu8->size, piu8->string);
	piu8->string[piu8->size] = 0;
	piu8->hdr.type = TAG_UTF8;
	fprintf(stderr, "TAG_UTF8: size:%u string:%s\n", piu8->size, piu8->string);
	break;
      case TAG_NAMEANDTYPE:
	pint = (JVMConstPoolNameAndType*)malloc(sizeof(JVMConstPoolNameAndType));
	pool[x] = (JVMConstPoolItem*)pint;
	pint->nameIndex = msRead16(m);
	pint->descIndex = msRead16(m);
	pint->hdr.type = TAG_NAMEANDTYPE;
	fprintf(stderr, "TAG_NAMEANDTYPE: nameIndex:%u descIndex:%u\n", pint->nameIndex,
		pint->descIndex);
	break;
      case TAG_FIELDREF:
	pifr = (JVMConstPoolFieldRef*)malloc(sizeof(JVMConstPoolFieldRef));
	pool[x] = (JVMConstPoolItem*)pifr;
	pifr->classIndex = msRead16(m);
	pifr->nameAndTypeIndex = msRead16(m);
	pifr->hdr.type = TAG_FIELDREF;
	fprintf(stderr, "classIndex:%u nameAndTypeIndex:%u\n", pifr->classIndex, 
		pifr->nameAndTypeIndex);
	break;
      default:
	fprintf(stderr, "unknown tag %u in constant pool\n\n", tag);
	exit(-1);
    }
  }
  /*
    ====================
    LOAD SMALL PARAMETERS
    ====================
  */
  class->accessFlags = msRead16(m);
  class->thisClass = msRead16(m);
  class->superClass = msRead16(m);
  /*
    =====================
    LOAD INTERFACES
    =====================
  */
  class->ifaceCnt = msRead16(m);
  printf("class->ifaceCnt:%u\n", class->ifaceCnt);
  class->interfaces = (uint16*)malloc(class->ifaceCnt);
  for (x = 0; x < class->ifaceCnt; ++x)
    class->interfaces[x] = msRead16(m);
  /*
    ======================
    LOAD FIELDS
    ======================
  */
  class->fieldCnt = msRead16(m);
  class->fields = (JVMClassField*)malloc(sizeof(JVMClassField) * class->fieldCnt);
  for (x = 0; x < class->fieldCnt; ++x) {
    class->fields[x].accessFlags = msRead16(m);
    class->fields[x].nameIndex = msRead16(m);
    class->fields[x].descIndex = msRead16(m);
    class->fields[x].attrCount = msRead16(m);
    fprintf(stderr, "accessFlags:%u nameIndex:%u descIndex:%u attrCount:%u\n",
	    class->fields[x].accessFlags,
	    class->fields[x].nameIndex,
	    class->fields[x].descIndex,
	    class->fields[x].attrCount
    );
  }
  /*
    =======================
    LOAD METHODS
    =======================
  */
  class->methodCnt = msRead16(m);
  class->methods = (JVMMethod*)malloc(sizeof(JVMMethod) * class->methodCnt);
  for (x = 0; x < class->methodCnt; ++x) {
    class->methods[x].accessFlags = msRead16(m);
    class->methods[x].nameIndex = msRead16(m);
    class->methods[x].descIndex = msRead16(m);
    class->methods[x].attrCount = msRead16(m);
    class->methods[x].attrs = (JVMAttribute*)malloc(sizeof(JVMAttribute) * 
      class->methods[x].attrCount);
    for (y = 0; y < class->methods[x].attrCount; ++y) {
      class->methods[x].attrs[y].nameIndex = msRead16(m);
      class->methods[x].attrs[y].length = msRead32(m);
      class->methods[x].attrs[y].info = (uint8*)malloc(
	      class->methods[x].attrs[y].length);
      fprintf(stderr, "name:%s\n", 
	      ((JVMConstPoolUtf8*)class->pool[class->methods[x].attrs[y].nameIndex - 1])->string
	      );
      fprintf(stderr, "attrlen:%u\n", class->methods[x].attrs[y].length);
      msRead(m, class->methods[x].attrs[y].length, class->methods[x].attrs[y].info);
    }
  }
  /*
    ======================
    LOAD ATTRIBUTES
    ======================
  */
  class->attrCnt = msRead16(m);
  class->attrs = (JVMAttribute*)malloc(sizeof(JVMAttribute) * class->attrCnt);
  for (x = 0; x < class->attrCnt; ++x) {
    class->attrs[x].nameIndex = msRead16(m);
    class->attrs[x].length = msRead32(m);
    class->attrs[x].info = (uint8*)malloc(class->attrs[x].length);
    msRead(m, class->attrs[x].length, class->attrs[x].info);
  }
  
  return class;
}

JVMClass* jvm_FindClassInBundle(JVMBundle *bundle, const char *className) {
  JVMBundleClass	        *jbclass;
  JVMConstPoolUtf8              *u;
  uint32                        a;
  JVMConstPoolClassInfo         *b;
  JVMConstPoolUtf8              *c;
  
  for (jbclass = bundle->first; jbclass != 0; jbclass = jbclass->next) {
    
    a = jbclass->jclass->thisClass;
    b = (JVMConstPoolClassInfo*)jbclass->jclass->pool[a - 1];
    c = (JVMConstPoolUtf8*)jbclass->jclass->pool[b->nameIndex - 1];
    printf("looking for class [%s]=?[%s]\n", className, c->string);
    if (strcmp(c->string, className) == 0)
      return jbclass->jclass;
  }
  /*
    This is where you want to do code to search externally to the
    bundle, and if needed load the missing class into memory.
  */
  return 0;
}

JVMMethod* jvm_FindMethodInClass(JVMClass *jclass, const char *methodName, const char *methodType) {
  JVMConstPoolUtf8      *a;
  JVMConstPoolUtf8      *b;
  int                   x;
  
  for (x = 0; x < jclass->methodCnt; ++x) {
    /// get method name
    a = (JVMConstPoolUtf8*)jclass->pool[jclass->methods[x].nameIndex - 1];
    /// get method type
    b = (JVMConstPoolUtf8*)jclass->pool[jclass->methods[x].descIndex - 1];
    fprintf(stderr, "findmeth:%s%s\n", a->string, b->string);
    if (strcmp(a->string, methodName) == 0)
      if (strcmp(b->string, methodType) == 0)
        return &jclass->methods[x];
  }
  fprintf(stderr, "could not find method %s of type %s\n", methodName, methodType);
  return 0;
}

int jvm_IsMethodReturnTypeVoid(const char *typestr) {
  int           x;

  /// go all the way to the return type part
  for (x = 0; typestr[x] != ')'; ++x);
  ++x;
  /// anything other than 'V' is non-void
  if (typestr[x] == 'V')
    return 1;
  return 0;
}

int jvm_GetMethodTypeArgumentCount(const char *typestr) {
  /*
      B byte
      C char
      D double
      F float
      I int
      J long
      L classname;
      S short
      Z boolean
  */
  int           x;
  int           c;
  
  /// chop off first parathesis
  typestr++;
  c = 0;
  /// read each type
  for (x = 0; typestr[x] != ')'; ++x) {
    switch (typestr[x]) {
      case 'L':
        c++;
        /// run until we find semi-colon
        for (++x; typestr[x] != ';' && x < 20; ++x);
        break;
      default:
        c++;
        break;
    }
  }

  return c;
}

static int g_dbg_ec = 0;

int jvm_ExecuteObjectMethod(JVM *jvm, JVMBundle *bundle, JVMClass *jclass,
                         const char *methodName, const char *methodType,
                         JVMLocal *_locals, uint8 localCnt, JVMLocal *_result) {
  JVMMethod                     *method;
  JVMConstPoolUtf8              *a;
  int                           x, y;
  JVMLocal                      *locals;
  uint32                        codesz;
  uint8                         *code;
  uint8                         opcode;
  JVMStack                      stack;
  JVMLocal                      result;

  JVMClass                      *_jclass;
  JVMObject                     *_jobject;
  JVMMethod                     *_method;
  JVMConstPoolMethodRef         *b;
  JVMConstPoolClassInfo         *c;
  JVMConstPoolNameAndType       *d;
  int                           argcnt;
  uint8                         *mclass;
  uint8                         *mmethod;
  uint8                         *mtype;
  uint8                         *tmp;

  //g_dbg_ec++;
  //if (g_dbg_ec == 3) {
  //  printf("g_dbg_ec=%u\n", g_dbg_ec);
  //  exit(-9);
  //}
  
  jvm_StackInit(&stack, 1024);
  
  printf("executing %s\n", methodName);
  /// -----------------------------------------------------
  /// i think there is a way to determine how much local
  /// variable space is needed... but for now this will work
  /// -----------------------------------------------------
  /// 255 should be maximum local addressable
  locals = (JVMLocal*)malloc(sizeof(JVMLocal) * 256);
  /// copy provided arguments into locals
  for (x = 0; x < localCnt; ++x) {
    locals[x].data = _locals[x].data;
    locals[x].flags = _locals[x].flags;
    printf("$$data:%u flags:%u\n", _locals[x].data, _locals[x].flags);
    if (locals[x].flags & JVM_STACK_ISOBJECTREF)
      ((JVMObject*)locals[x].data)->stackCnt++;
  }
  /// zero the remainder of the locals
  for (; x < 256; ++x) {
    locals[x].data = 0;
    locals[x].flags = 0;
  }
  /// find method specifiee
  method = jvm_FindMethodInClass(jclass, methodName, methodType);
  if (!method) {
    fprintf(stderr, "JVM_ERROR_METHODNOTFOUND");
    return JVM_ERROR_METHODNOTFOUND;
  }
  /// find code attribute
  for (x = 0; x < method->attrCount; ++x) {
    a = (JVMConstPoolUtf8*)jclass->pool[method->attrs[x].nameIndex - 1];
    if (strcmp(a->string, "Code") == 0) {
      code = method->attrs[x].info;
      codesz = method->attrs[x].length;
      break;
    }
  }

  printf("execute code\n");
  /// execute code
  for (x = 0; x < codesz;) {
    opcode = code[x];
    printf("opcode(%u/%u):%x\n", x, codesz, opcode);
    switch (opcode) {
      /// nop: no operation
      case 0:
        x += 2;
        break;
      /// bipush: push a byte onto the stack as an integer
      case 0x10:
        y = code[x+1];
        jvm_StackPush(&stack, y, 0);
        x += 2;
        break;
      /// pop: discard top value on stack
      case 0x57:
        jvm_StackPop(&stack, &result);
        if (result.flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)result.data)->stackCnt--;
        x += 1;
        break;
      /// aload
      case 0x19:
        y = code[x+1];
        jvm_StackPush(&stack, locals[y].data, locals[y].flags);
        if (locals[x].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[0].data)->stackCnt++;
        x += 2;
        break;
      /// aload_0: load a reference onto the stack from local variable 0
      case 0x2a:
        jvm_StackPush(&stack, locals[0].data, locals[0].flags);
        if (locals[0].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[0].data)->stackCnt++;
        x += 1;
        break;
      /// aload_1
      case 0x2b:
        jvm_StackPush(&stack, locals[1].data, locals[1].flags);
        if (locals[1].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[1].data)->stackCnt++;
        x += 1;
        break;
      /// aload_2
      case 0x2c:
        jvm_StackPush(&stack, locals[2].data, locals[2].flags);
        if (locals[2].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[2].data)->stackCnt++;
        x += 1;
        break;
      /// aload_3
      case 0x2d:
        jvm_StackPush(&stack, locals[3].data, locals[3].flags);
        if (locals[3].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[3].data)->stackCnt++;
        x += 1;
        break;
      /// invokevirtual
      case 0xb6:
      /// invokespecial
      case 0xb7:
         /*
            (1) verify objref is indeed an object reference
                use the type info on the stack
            (2) verify objref is a reference to the described object
            (3) verify the number of arguments are correct
         */
         y = code[x+1] << 8 | code[x+2];

         b = (JVMConstPoolMethodRef*)jclass->pool[y - 1];
         c = (JVMConstPoolClassInfo*)jclass->pool[b->nameIndex - 1];
         a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
         // a->string is className of class we are calling method on
         mclass = a->string;

         /// if java/lang/Object just pretend we did
         if (strcmp(mclass, "java/lang/Object") == 0) {
          printf("caught java/lang/Object call and skipped it\n");
          x +=3 ;
          break;
         }
         
         d = (JVMConstPoolNameAndType*)jclass->pool[b->descIndex - 1];
         a = (JVMConstPoolUtf8*)jclass->pool[d->nameIndex - 1];
         // a->string is the method of the class
         _jclass = jvm_FindClassInBundle(bundle, mclass);

         mmethod = a->string;

         a = (JVMConstPoolUtf8*)jclass->pool[d->descIndex - 1];
         // a->string is the type description of the method
         mtype = a->string;

         /// look for method in our class then walk super classes
         /// and see if we can find it
         while ((_method = jvm_FindMethodInClass(_jclass, mmethod, mtype)) == 0) {
           c = (JVMConstPoolClassInfo*)_jclass->pool[_jclass->superClass - 1];
           a = (JVMConstPoolUtf8*)_jclass->pool[c->nameIndex - 1];
           _jclass = jvm_FindClassInBundle(bundle, a->string);
           if (jclass == 0)
           {
             fprintf(stderr, "Could not find super class in bundle!?!\n");
             exit(-9);
           }
         }
         printf("class with implementation is %s\n", a->string);
         
         
         argcnt = jvm_GetMethodTypeArgumentCount(mtype);
         
         printf("invokespecial: %s:%s[%u] in %s\n", mmethod, mtype, argcnt, mclass);

         jvm_DebugStack(&stack);
         
         /// pop locals from stack into local variable array
         _locals = (JVMLocal*)malloc(sizeof(JVMLocal) * (argcnt + 2));
         for (y = 0; y < argcnt; ++y) {
           jvm_StackPop(&stack, &result); 
           _locals[y + 1].data = result.data;
           _locals[y + 1].flags = result.flags;
         }
         /// pop object reference from stack
         jvm_StackPop(&stack, &result);
         if (!(result.flags & JVM_STACK_ISOBJECTREF)) {
           fprintf(stderr, "object from stack is not object reference!");
           exit(-8);
         }
         
         /// stays the same since we poped it then placed it into locals
         //((JVMObject*)result.data)->stackCnt

         _locals[0].data = result.data;
         _locals[0].flags = result.flags;

         printf("objref->stackCnt:%u\n", ((JVMObject*)_locals[0].data)->stackCnt);
         printf("calling method with self.data:%x self.flags:%u\n", _locals[0].data, _locals[0].flags);

         printf("########################");
         jvm_ExecuteObjectMethod(jvm, bundle, _jclass, mmethod, mtype, locals, argcnt + 1, &result);
         free(_locals);

         printf("@@@@@@@@@@@%s\n", mtype);

         /// need to know if it was a void return or other
         if (!jvm_IsMethodReturnTypeVoid(mtype)) {
          /// push result onto stack
          printf("return type not void!\n");
          jvm_StackPush(&stack, result.data, result.flags);
         } else {
           printf("return type void..\n");
         }
         
         x += 3;
         break;
      /// ireturn: return integer from method
      case 0xac:
      /// return: void from method
      printf("return integer found\n");
      case 0xb1:
        if (opcode == 0xac)
        {
          /// no check if objref and decrement refcnt because it
          /// is going right back on the stack we were called from
          jvm_DebugStack(&stack);
          jvm_StackPop(&stack, _result);
        }
        
         /// need to go through and decrement reference count of objects on stack and local
         /// the stack should be empty right? maybe not...
         printf("checking for stuff left on stack after method return..\n");
         while (jvm_StackMore(&stack)) {
           jvm_StackPop(&stack, &result);
           if (result.flags & JVM_STACK_ISOBJECTREF) {
             fprintf(stderr, "function return void decrement stack object ref %u\n", ((JVMObject*)result.data)->stackCnt);
             ((JVMObject*)result.data)->stackCnt--;
           }
         }
         printf("checking for stuff left in local variables after method return..\n");
         /// also do local variables the same, decrement obj references
         for (y = 0; y < 256; ++y) {
           if (locals[y].flags & JVM_STACK_ISOBJECTREF) {
              fprintf(stderr, "function return void decrement local object ref %u\n", ((JVMObject*)locals[y].data)->stackCnt);
              ((JVMObject*)locals[y].data)->stackCnt--;
           }
         }
         printf("actually returning from method..\n");
         return JVM_SUCCESS;
      default:
        fprintf(stderr, "unknown opcode %x\n", opcode);
        exit(-3);
        return JVM_ERROR_UNKNOWNOPCODE;
    }
  }
  
  return JVM_SUCCESS;
}

int jvm_CreateObject(JVM *jvm, JVMBundle *bundle, const char *className, JVMObject **out) {
  JVMClass                      *jclass;
  JVMObject                     *jobject;
  JVMLocal                      result;
  JVMLocal                      locals[1];
  
  *out = 0;
  /// find class and create instance
  printf("create object with class %s\n", className);
  jclass = jvm_FindClassInBundle(bundle, className);
  if (!jclass)
    return JVM_ERROR_CLASSNOTFOUND;
  jobject = (JVMObject*)malloc(sizeof(JVMObject));
  *out = jobject;
  if (!jobject)
    return JVM_ERROR_OUTOFMEMORY;
  memset(*out, 0, sizeof(JVMObject));
  jobject->class = jclass;
  /// link us into global object chain
  jobject->next = jvm->objects;
  jvm->objects = jobject;
  /// execute init method
  locals[0].data = (uint64)jobject;
  locals[0].flags = JVM_STACK_ISOBJECTREF;
  /// call default constructor (no arguments)
  return jvm_ExecuteObjectMethod(jvm, bundle, jclass, "<init>", "()V", &locals[0], 1, &result);
}

uint8* jvm_ReadWholeFile(const char *path, uint32 *size) {
  uint8         *buf;
  FILE          *fp;
  
  fp = fopen(path, "rb");
  fseek(fp, 0, 2);
  *size = ftell(fp);
  fseek(fp, 0, 0);
  buf = (uint8*)malloc(*size);
  fread(buf, 1, *size, fp);
  fclose(fp);
  return buf;
}

void jvm_AddClassToBundle(JVMBundle *jbundle, JVMClass *jclass) {
  JVMBundleClass                *jbclass;

  jbclass = (JVMBundleClass*)malloc(sizeof(JVMBundleClass));
  jbclass->jclass = jclass;

  jbclass->next = jbundle->first;
  jbundle->first = jbclass;

  return;
}

int main(int argc, char *argv[])
{
  uint8			*buf;
  JVMMemoryStream	m;
  JVMClass		*jclass;
  JVMBundle		jbundle;
  JVMBundleClass	*jbclass;
  JVM                   jvm;
  JVMObject             *jobject;
  JVMLocal              locals[10];
  uint32                size;
  int                   result;
  JVMLocal              jvm_result;

  buf = jvm_ReadWholeFile("Apple.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);
  
  buf = jvm_ReadWholeFile("Test.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);

  jvm.objects = 0;

  /// create initial object
  result = jvm_CreateObject(&jvm, &jbundle, "Test", &jobject);

  locals[0].data = (uint64)jobject;
  locals[0].flags = JVM_STACK_ISOBJECTREF;
  jvm_ExecuteObjectMethod(&jvm, &jbundle, jclass, "main", "()I", &locals[0], 1, &jvm_result);
  printf("done! result.data:%lu result.flags:%u\n", jvm_result.data, jvm_result.flags);
  return 1;
}
