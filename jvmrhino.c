#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "stack.h"
#include "exec.h"
#include "ms.h"

JVMClass* jvm_LoadClass(JVMMemoryStream *m) {
  uint16			vmin;
  uint16			vmaj;
  uint32			magic;
  uint16			cpoolcnt;
  uint8				tag;
  JVMConstPoolItem		**pool;
  JVMConstPoolItem		*pi;
  int				x, y, z;
  JVMConstPoolMethodRef		*pimr;
  JVMConstPoolClassInfo		*pici;
  JVMConstPoolUtf8		*piu8;
  JVMConstPoolNameAndType	*pint;
  JVMConstPoolFieldRef		*pifr;
  JVMConstPoolString            *pist;
  JVMClass			*class;
  uint8                         *string;
  
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
      case TAG_STRING:
        pist = (JVMConstPoolString*)malloc(sizeof(JVMConstPoolString));
        pool[x] = (JVMConstPoolItem*)pist;
        pist->stringIndex = msRead16(m);
        pist->hdr.type = TAG_STRING;
        debugf("TAG_STRING stringIndex:%u\n", pist->stringIndex);
        break;
      case TAG_METHODREF:
	pimr = (JVMConstPoolMethodRef*)malloc(sizeof(JVMConstPoolMethodRef));
	pool[x] = (JVMConstPoolItem*)pimr;
	pimr->nameIndex = msRead16(m);
	pimr->descIndex = msRead16(m);
	pimr->hdr.type = TAG_METHODREF;
	debugf("TAG_METHODREF nameIndex:%u descIndex:%u\n", pimr->nameIndex, pimr->descIndex);
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
	debugf("TAG_UTF8: size:%u string:%s\n", piu8->size, piu8->string);
	break;
      case TAG_NAMEANDTYPE:
	pint = (JVMConstPoolNameAndType*)malloc(sizeof(JVMConstPoolNameAndType));
	pool[x] = (JVMConstPoolItem*)pint;
	pint->nameIndex = msRead16(m);
	pint->descIndex = msRead16(m);
	pint->hdr.type = TAG_NAMEANDTYPE;
	debugf("TAG_NAMEANDTYPE: nameIndex:%u descIndex:%u\n", pint->nameIndex,
		pint->descIndex);
	break;
      case TAG_FIELDREF:
	pifr = (JVMConstPoolFieldRef*)malloc(sizeof(JVMConstPoolFieldRef));
	pool[x] = (JVMConstPoolItem*)pifr;
	pifr->classIndex = msRead16(m);
	pifr->nameAndTypeIndex = msRead16(m);
	pifr->hdr.type = TAG_FIELDREF;
	debugf("classIndex:%u nameAndTypeIndex:%u\n", pifr->classIndex,
		pifr->nameAndTypeIndex);
	break;
      default:
	debugf("unknown tag %u in constant pool\n\n", tag);
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
    /// in the event it has no actual code attribute
    class->methods[x].code = 0;
    class->methods[x].accessFlags = msRead16(m);
    class->methods[x].nameIndex = msRead16(m);
    class->methods[x].descIndex = msRead16(m);
    class->methods[x].attrCount = msRead16(m);
    class->methods[x].attrs = (JVMAttribute*)malloc(sizeof(JVMAttribute) * 
      class->methods[x].attrCount);
    for (y = 0; y < class->methods[x].attrCount; ++y) {
      class->methods[x].attrs[y].nameIndex = msRead16(m);
      class->methods[x].attrs[y].length = msRead32(m);

      string = ((JVMConstPoolUtf8*)class->pool[class->methods[x].attrs[y].nameIndex - 1])->string;
      debugf("name:%s\n", string);
      debugf("attrlen:%u\n", class->methods[x].attrs[y].length);
      if (strcmp(string, "Code") == 0) {
        /// special attribute we need to fully parse out
        class->methods[x].code = (JVMCodeAttribute*)malloc(sizeof(JVMCodeAttribute));
        class->methods[x].code->attrNameIndex = class->methods[x].attrs[y].nameIndex;
        class->methods[x].code->attrLength = class->methods[x].attrs[y].length;
        /// not used but lets set it up to be safe incase accidentally accessed
        class->methods[x].attrs[y].nameIndex = 0;
        class->methods[x].attrs[y].length = 0;
        class->methods[x].attrs[y].info = 0;
        /// read parameters
        class->methods[x].code->maxStack = msRead16(m);
        class->methods[x].code->maxLocals = msRead16(m);
        class->methods[x].code->codeLength = msRead32(m);
        /// read code segment
        class->methods[x].code->code = (uint8*)malloc(class->methods[x].code->codeLength);
        msRead(m, class->methods[x].code->codeLength, class->methods[x].code->code);
        /// read exception table
        class->methods[x].code->eTableCount = msRead16(m);
        class->methods[x].code->eTable = 0;
        if (class->methods[x].code->eTableCount) {
          class->methods[x].code->eTable = (JVMExceptionTable*)malloc(sizeof(JVMExceptionTable) * class->methods[x].code->eTableCount);
          for (z = 0; z < class->methods[x].code->eTableCount; z++) {
            class->methods[x].code->eTable[z].pcStart = msRead16(m);
            class->methods[x].code->eTable[z].pcEnd = msRead16(m);
            class->methods[x].code->eTable[z].pcHandler = msRead16(m);
            class->methods[x].code->eTable[z].catchType = msRead16(m);
          }
        }
        debugf("eTableCount:%u\n", class->methods[x].code->eTableCount);
        class->methods[x].code->attrsCount = msRead16(m);
        class->methods[x].code->attrs = 0;
        if (class->methods[x].code->attrsCount) {
          class->methods[x].code->attrs = (JVMAttribute*)malloc(sizeof(JVMAttribute) * class->methods[x].code->attrsCount);
          /// read attributes for just this code not method?
          debugf("class->methods[x].code->attrsCount:%u\n", class->methods[x].code->attrsCount);
          for (z = 0; z < class->methods[x].code->attrsCount; ++z) {
            debugf("yyyy\n");
            class->methods[x].code->attrs[z].nameIndex = msRead16(m);
            class->methods[x].code->attrs[z].length = msRead32(m);
            class->methods[x].code->attrs[z].info = (uint8*)malloc(class->methods[x].code->attrs[z].length);
            msRead(m, class->methods[x].code->attrs[z].length, class->methods[x].code->attrs[z].info);
            debugf("xxxx\n");
          }
        }
        debugf("done reading code attribute");
      } else {
        /// standard operation for attributes we do not really understand
        class->methods[x].attrs[y].info = (uint8*)malloc(class->methods[x].attrs[y].length);
        msRead(m, class->methods[x].attrs[y].length, class->methods[x].attrs[y].info);
      }
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
    debugf("looking for class [%s]=?[%s]\n", className, c->string);
    if (strcmp(c->string, className) == 0)
      return jbclass->jclass;
  }
  /*
    This is where you want to do code to search externally to the
    bundle, and if needed load the missing class into memory.
  */
  debugf("class [%s] not found in bundle!\n", className);
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
    debugf("findmeth:[%s:%s]==[%s:%s]\n", methodName, methodType, a->string, b->string);
    if (strcmp(a->string, methodName) == 0)
      if (strcmp(b->string, methodType) == 0)
        return &jclass->methods[x];
  }
  debugf("could not find method %s of type %s\n", methodName, methodType);
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

void jvm_ScrubObjectFields(JVMObject *jobject) {
  int   w;
  for (w = 0; w < jobject->fieldCnt; ++w) {
    if (jobject->_fields[w].aflags & JVM_STACK_ISOBJECTREF)
      if (jobject->_fields[w].value)
        ((JVMObject*)jobject->_fields[w].value)->stackCnt--;
  }
}

void jvm_ScrubLocals(JVMLocal *locals) {
  int           y;
  for (y = 0; y < 256; ++y) {
    if (locals[y].flags & JVM_STACK_ISOBJECTREF) {
      ((JVMObject*)locals[y].data)->stackCnt--;
      debugf("SCRUB LOCALS ref:%lx refcnt:%i\n", locals[y].data, ((JVMObject*)locals[y].data)->stackCnt);
    }
  }
}
void jvm_ScrubStack(JVMStack *stack) {
  JVMLocal              result;
  while (jvm_StackMore(stack)) {
    jvm_StackPop(stack, &result);
  }
}

/// helper function
uint8* jvm_GetClassNameFromClass(JVMClass *c) {
  JVMConstPoolClassInfo         *ci;
  JVMConstPoolUtf8              *u;
  ci = (JVMConstPoolClassInfo*)c->pool[c->thisClass - 1];
  u = (JVMConstPoolUtf8*)c->pool[ci->nameIndex - 1];
  return u->string;
}

/// is jobject an instance of the specified classname
int jvm_IsInstanceOf(JVMBundle *bundle, JVMObject *jobject, uint8 *className) {
  /// jvm_FindClassInBundle
  JVMClass                      *c;
  JVMConstPoolClassInfo         *ci;
  JVMConstPoolUtf8              *u8;
  
  c = jobject->class;
  while (1) {
    /// check c's class name with class name
    debugf("ok %x\n", jobject->class);
    ci = (JVMConstPoolClassInfo*)c->pool[c->thisClass - 1];
    u8 = (JVMConstPoolUtf8*)c->pool[ci->nameIndex - 1];

    if (strcmp((const char*)u8->string, className) == 0)
      return 1;
      /// if equals exit with true
      
    /// no super class exit with false
    if (strcmp(u8->string, "java/lang/Object") == 0)
      break;
    /// load class for specified super class
    ci = (JVMConstPoolClassInfo*)c->pool[c->superClass - 1];
    u8 = (JVMConstPoolUtf8*)c->pool[ci->nameIndex - 1];
    /// we got an error just not sure best way to handle it
    c = jvm_FindClassInBundle(bundle, u8->string);
    if (!c)
      break;
  }

  return 0;
}

int jvm_CountObjectFields(JVM *jvm, JVMBundle *bundle, JVMObject *jobject) {
  JVMClass                      *c;
  JVMConstPoolClassInfo         *ci;
  JVMConstPoolUtf8              *u8;
  int                           x;
  
  c = jobject->class;
  x = 0;
  while (1) {
    /// create fields
    x += c->fieldCnt;

    /// load class for specified super class
    if (!c->superClass) {
      break;
    }
    ci = (JVMConstPoolClassInfo*)c->pool[c->superClass - 1];
    u8 = (JVMConstPoolUtf8*)c->pool[ci->nameIndex - 1];
    /// we got an error just not sure best way to handle it
    c = jvm_FindClassInBundle(bundle, u8->string);
    if (!c) {
      debugf("missing super class! [%s]\n", u8->string);
      break;
    }
  }

  return x;
}

int jvm_FieldTypeStringToFlags(JVMBundle *bundle, uint8 *typestr, JVMClass **class, uint32 *flags) {
  int                   x, y;
  uint8                 buf[128];
  
  *flags = 0;
  for (x = 0; typestr[x] != 0; ++x) {
    switch(typestr[x]) {
      case '[':
        *flags |= JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF;
        break;
      case 'B':
        *flags |= JVM_STACK_ISBYTE;
        break;
      case 'C':
        *flags |= JVM_STACK_ISCHAR;
        break;
      case 'D':
        *flags |= JVM_STACK_ISDOUBLE;
        break;
      case 'F':
        *flags |= JVM_STACK_ISFLOAT;
        break;
      case 'I':
        *flags |= JVM_STACK_ISINT;
        break;
      case 'J':
        *flags |= JVM_STACK_ISLONG;
        break;
      case 'L':
        *flags |= JVM_STACK_ISOBJECTREF;
        for (y = x + 1; typestr[y] != ';'; ++y) {
          buf[y - x - 1] = typestr[y];
        }
        buf[y - x - 1] = 0;
        *class = jvm_FindClassInBundle(bundle, &buf[0]);
        if (!*class) {
          debugf("could not find class in bundle\n");
          return JVM_ERROR_CLASSNOTFOUND;
        }
        break;
      case 'S':
        *flags |= JVM_STACK_ISSHORT;
        break;
      case 'Z':
        *flags |= JVM_STACK_ISBOOL;
        break;
    }
  }

  return JVM_SUCCESS;
}

int jvm_MakeObjectFields(JVM *jvm, JVMBundle *bundle, JVMObject *jobject) {
  JVMClass                      *c;
  JVMConstPoolClassInfo         *ci;
  JVMConstPoolUtf8              *u8;
  JVMObjectField                *fields;
  JVMClass                      *jclass;
  int                           fcnt;
  int                           x;
  int                           y;
  int                           error;
  
  fcnt = jvm_CountObjectFields(jvm, bundle, jobject);
  fields = (JVMObjectField*)malloc(sizeof(JVMObjectField) * fcnt);
  jobject->fieldCnt = fcnt;
  jobject->_fields = fields;

  y = 0;
  c = jobject->class;
  while (1) {
    /// create fields
    for (x = 0; x < c->fieldCnt; ++x) {
      // get field name
      u8 = (JVMConstPoolUtf8*)c->pool[c->fields[x].nameIndex - 1];
      fields[y].name = u8->string;
      // get field type
      u8 = (JVMConstPoolUtf8*)c->pool[c->fields[x].descIndex - 1];
      // convert into flags
      error = jvm_FieldTypeStringToFlags(bundle, u8->string, &jclass, &fields[y].flags);
      debugf("made field %s<%s> flags:%u\n", fields[y].name, u8->string, fields[y].flags);
      // for object arrays this is needed (not primitive arrays)
      fields[y].jclass = jclass;
      // valid for all types
      fields[y].value = 0;
      ++y;
    } 

    /// load class for specified super class
    if (!c->superClass) {
      break;
    }
    ci = (JVMConstPoolClassInfo*)c->pool[c->superClass - 1];
    u8 = (JVMConstPoolUtf8*)c->pool[ci->nameIndex - 1];
    /// we got an error just not sure best way to handle it
    c = jvm_FindClassInBundle(bundle, u8->string);
    if (!c) {
      debugf("missing super class! [%s]\n", u8->string);
      return -1;
    }
  }

  return 1;
}

int jvm_CreateObject(JVM *jvm, JVMBundle *bundle, const char *className, JVMObject **out) {
  JVMClass                      *jclass;
  JVMObject                     *jobject;
  JVMLocal                      result;
  JVMLocal                      locals[1];
  int                           fcnt;
  
  *out = 0;
  /// find class and create instance
  debugf("create object with class %s\n", className);
  jclass = jvm_FindClassInBundle(bundle, className);
  if (!jclass)
  {
    debugf("class not found!\n");
    return JVM_ERROR_CLASSNOTFOUND;
  }
  jobject = (JVMObject*)malloc(sizeof(JVMObject));
  *out = jobject;
  if (!jobject)
    return JVM_ERROR_OUTOFMEMORY;
  memset(*out, 0, sizeof(JVMObject));
  jobject->class = jclass;
  jobject->stackCnt = 0;
  /// link us into global object chain
  jobject->next = jvm->objects;
  jvm->objects = jobject;
  /// go through and create fields
  //debugf("WWW\n");
  jvm_MakeObjectFields(jvm, bundle, jobject);
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

  buf = jvm_ReadWholeFile("./java/lang/Array.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);
  
  buf = jvm_ReadWholeFile("./java/lang/Toodle.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);
  
  buf = jvm_ReadWholeFile("./java/lang/Object.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);

  buf = jvm_ReadWholeFile("./java/lang/Exception.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);

  buf = jvm_ReadWholeFile("Peach.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);
  
  buf = jvm_ReadWholeFile("Grape.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jvm_AddClassToBundle(&jbundle, jclass);
  
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
  result = jvm_ExecuteObjectMethod(&jvm, &jbundle, jclass, "main", "()I", &locals[0], 1, &jvm_result);
  if (result < 0) {
    debugf("error occured in execution somewhere code:%i\n", result);
    return -1;
  }
  debugf("done! result.data:%li result.flags:%u\n", jvm_result.data, jvm_result.flags);

  debugf("---dumping objects---\n");
  for (jobject = jvm.objects; jobject != 0; jobject = jobject->next) {
    debugf("jobject:%x\tstackCnt:%i\tclassName:%s\n", jobject, jobject->stackCnt, jvm_GetClassNameFromClass(jobject->class));
  }
  return 1;
}
