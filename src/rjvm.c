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
  JVMConstPoolInteger           *piit;
  JVMConstPoolLong              *pllt;
  JVMConstPoolIntMethRef        *imr;
  JVMClass			*class;
  uint8                         *string;
  
  magic = msRead32(m);
  vmin = msRead16(m);
  vmaj = msRead16(m);
  
  class = (JVMClass*)jvm_malloc(sizeof(JVMClass));
  /*
    ==================================
    LOAD CONSTANT POOL TABLE
    ==================================
  */
  cpoolcnt = msRead16(m); 
  pool = (JVMConstPoolItem**)jvm_malloc(sizeof(JVMConstPoolItem*) * cpoolcnt);
  class->poolCnt = cpoolcnt;
  class->pool = pool;
  //debugf("cpoolcnt:%u", cpoolcnt);
  for (x = 0; x < cpoolcnt - 1; ++x) {
    tag = msRead8(m);
    debugf("tag[%u/%u]:%u\n", x, cpoolcnt, tag);
    switch (tag) {
      case TAG_LONG:
        pllt = (JVMConstPoolLong*)jvm_malloc(sizeof(JVMConstPoolLong));
        pool[x+0] = (JVMConstPoolItem*)pllt;
        pool[x+1] = (JVMConstPoolItem*)pllt;
        pllt->high = msRead32(m);
        pllt->low = msRead32(m);
        pllt->hdr.type = TAG_LONG;
        debugf("TAG_LONG high:%x low:%x\n", pllt->high, pllt->low);
        ++x;
        break;
      case TAG_INTEGER:
        piit = (JVMConstPoolInteger*)jvm_malloc(sizeof(JVMConstPoolInteger));
        pool[x] = (JVMConstPoolItem*)piit;
        piit->value = msRead32(m);
        piit->hdr.type = TAG_INTEGER;
        break;
      case TAG_STRING:
        pist = (JVMConstPoolString*)jvm_malloc(sizeof(JVMConstPoolString));
        pool[x] = (JVMConstPoolItem*)pist;
        pist->stringIndex = msRead16(m);
        pist->hdr.type = TAG_STRING;
        break;
      case TAG_METHODREF:
	pimr = (JVMConstPoolMethodRef*)jvm_malloc(sizeof(JVMConstPoolMethodRef));
	pool[x] = (JVMConstPoolItem*)pimr;
	pimr->nameIndex = msRead16(m);
	pimr->descIndex = msRead16(m);
	pimr->hdr.type = TAG_METHODREF;
	//debugf("TAG_METHODREF nameIndex:%u descIndex:%u\n", pimr->nameIndex, pimr->descIndex);
	break;
      case TAG_CLASSINFO:
	pici = (JVMConstPoolClassInfo*)jvm_malloc(sizeof(JVMConstPoolClassInfo));
	pool[x] = (JVMConstPoolItem*)pici;
	pici->nameIndex = msRead16(m);
	pici->hdr.type = TAG_CLASSINFO;
	break;
      case TAG_UTF8:
	piu8 = (JVMConstPoolUtf8*)jvm_malloc(sizeof(JVMConstPoolUtf8));
	pool[x] = (JVMConstPoolItem*)piu8;
	piu8->size = msRead16(m);
	piu8->string = (uint8*)jvm_malloc(piu8->size + 1);
	msRead(m, piu8->size, piu8->string);
	piu8->string[piu8->size] = 0;
	piu8->hdr.type = TAG_UTF8;
	debugf("TAG_UTF8: size:%u string:%s\n", piu8->size, piu8->string);
	break;
      case TAG_INTMETHREF:
  imr = (JVMConstPoolIntMethRef*)jvm_malloc(sizeof(JVMConstPoolIntMethRef));
  pool[x] = (JVMConstPoolItem*)imr;
  imr->classIndex = msRead16(m);
  imr->nameAndTypeIndex = msRead16(m);
  imr->hdr.type = TAG_INTMETHREF;
  break;
      case TAG_NAMEANDTYPE:
	pint = (JVMConstPoolNameAndType*)jvm_malloc(sizeof(JVMConstPoolNameAndType));
	pool[x] = (JVMConstPoolItem*)pint;
	pint->nameIndex = msRead16(m);
	pint->descIndex = msRead16(m);
	pint->hdr.type = TAG_NAMEANDTYPE;
	//debugf("TAG_NAMEANDTYPE: nameIndex:%u descIndex:%u\n", pint->nameIndex, pint->descIndex);
	break;
      case TAG_FIELDREF:
	pifr = (JVMConstPoolFieldRef*)jvm_malloc(sizeof(JVMConstPoolFieldRef));
	pool[x] = (JVMConstPoolItem*)pifr;
	pifr->classIndex = msRead16(m);
	pifr->nameAndTypeIndex = msRead16(m);
	pifr->hdr.type = TAG_FIELDREF;
	//debugf("classIndex:%u nameAndTypeIndex:%u\n", pifr->classIndex, pifr->nameAndTypeIndex);
	break;
      default:
	debugf("unknown tag %u in constant pool\n\n", tag);
	jvm_exit(-1);
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
  jvm_printf("class->ifaceCnt:%u\n", class->ifaceCnt);
  class->interfaces = (uint16*)jvm_malloc(class->ifaceCnt);
  for (x = 0; x < class->ifaceCnt; ++x)
    class->interfaces[x] = msRead16(m);
  /*
    ======================
    LOAD FIELDS
    ======================
  */
  class->fieldCnt = msRead16(m);
  class->fields = (JVMClassField*)jvm_malloc(sizeof(JVMClassField) * class->fieldCnt);
  for (x = 0; x < class->fieldCnt; ++x) {
    class->fields[x].accessFlags = msRead16(m);
    class->fields[x].nameIndex = msRead16(m);
    class->fields[x].descIndex = msRead16(m);
    class->fields[x].attrCount = msRead16(m);
    if (class->fields[x].attrCount > 0) {
        class->fields[x].attrs = (JVMAttribute*)jvm_malloc(sizeof(JVMAttribute) * class->fields[x].attrCount);
        for (y = 0; y < class->fields[x].attrCount; ++y) {
          class->fields[x].attrs[y].nameIndex = msRead16(m);
          class->fields[x].attrs[y].length = msRead32(m);
          class->fields[x].attrs[y].info = (uint8*)jvm_malloc(sizeof(uint8) * class->fields[x].attrs[y].length);
          msRead(m, class->fields[x].attrs[y].length, class->fields[x].attrs[y].info);
          debugf("read field attribute of length %u\n", class->fields[x].attrs[y].length);
        }
    } else
      class->fields[x].attrs = 0;
    //debugf("accessFlags:%u nameIndex:%u descIndex:%u attrCount:%u\n", class->fields[x].accessFlags, class->fields[x].nameIndex, class->fields[x].descIndex,class->fields[x].attrCount);
  }
  /*
    =======================
    LOAD METHODS
    =======================
  */
  class->methodCnt = msRead16(m);
  class->methods = (JVMMethod*)jvm_malloc(sizeof(JVMMethod) * class->methodCnt);
  for (x = 0; x < class->methodCnt; ++x) {
    /// in the event it has no actual code attribute
    class->methods[x].code = 0;
    class->methods[x].accessFlags = msRead16(m);
    class->methods[x].nameIndex = msRead16(m);
    class->methods[x].descIndex = msRead16(m);
    class->methods[x].attrCount = msRead16(m);

    //debugf("--------------->method:%s\n", ((JVMConstPoolUtf8*)class->pool[class->methods[x].nameIndex - 1])->string);
    //debugf("--------------->desc:%s\n", ((JVMConstPoolUtf8*)class->pool[class->methods[x].descIndex - 1])->string);
    
    class->methods[x].attrs = (JVMAttribute*)jvm_malloc(sizeof(JVMAttribute) *
      class->methods[x].attrCount);
    for (y = 0; y < class->methods[x].attrCount; ++y) {
      class->methods[x].attrs[y].nameIndex = msRead16(m);
      class->methods[x].attrs[y].length = msRead32(m);

      string = ((JVMConstPoolUtf8*)class->pool[class->methods[x].attrs[y].nameIndex - 1])->string;
      //debugf("name:%s\n", string);
      //debugf("attrlen:%u\n", class->methods[x].attrs[y].length);
      if (jvm_strcmp(string, "Code") == 0) {
        /// special attribute we need to fully parse out
        class->methods[x].code = (JVMCodeAttribute*)jvm_malloc(sizeof(JVMCodeAttribute));
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
        class->methods[x].code->code = (uint8*)jvm_malloc(class->methods[x].code->codeLength);
        msRead(m, class->methods[x].code->codeLength, class->methods[x].code->code);
        /// read exception table
        class->methods[x].code->eTableCount = msRead16(m);
        class->methods[x].code->eTable = 0;
        if (class->methods[x].code->eTableCount) {
          class->methods[x].code->eTable = (JVMExceptionTable*)jvm_malloc(sizeof(JVMExceptionTable) * class->methods[x].code->eTableCount);
          for (z = 0; z < class->methods[x].code->eTableCount; z++) {
            class->methods[x].code->eTable[z].pcStart = msRead16(m);
            class->methods[x].code->eTable[z].pcEnd = msRead16(m);
            class->methods[x].code->eTable[z].pcHandler = msRead16(m);
            class->methods[x].code->eTable[z].catchType = msRead16(m);
          }
        }
        //debugf("eTableCount:%u\n", class->methods[x].code->eTableCount);
        class->methods[x].code->attrsCount = msRead16(m);
        class->methods[x].code->attrs = 0;
        if (class->methods[x].code->attrsCount) {
          class->methods[x].code->attrs = (JVMAttribute*)jvm_malloc(sizeof(JVMAttribute) * class->methods[x].code->attrsCount);
          /// read attributes for just this code not method?
          //debugf("class->methods[x].code->attrsCount:%u\n", class->methods[x].code->attrsCount);
          for (z = 0; z < class->methods[x].code->attrsCount; ++z) {
            class->methods[x].code->attrs[z].nameIndex = msRead16(m);
            class->methods[x].code->attrs[z].length = msRead32(m);
            class->methods[x].code->attrs[z].info = (uint8*)jvm_malloc(class->methods[x].code->attrs[z].length);
            msRead(m, class->methods[x].code->attrs[z].length, class->methods[x].code->attrs[z].info);
          }
        }
        //debugf("done reading code attribute\n");
      } else {
        /// standard operation for attributes we do not really understand
        class->methods[x].attrs[y].info = (uint8*)jvm_malloc(class->methods[x].attrs[y].length);
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
  class->attrs = (JVMAttribute*)jvm_malloc(sizeof(JVMAttribute) * class->attrCnt);
  for (x = 0; x < class->attrCnt; ++x) {
    class->attrs[x].nameIndex = msRead16(m);
    class->attrs[x].length = msRead32(m);
    class->attrs[x].info = (uint8*)jvm_malloc(class->attrs[x].length);
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
    debugf("a:%x\n", b);
    c = (JVMConstPoolUtf8*)jbclass->jclass->pool[b->nameIndex - 1];
    debugf("right before..\n");
    debugf("looking for class [%s]=?[%s]\n", className, c->string);
    if (jvm_strcmp(c->string, className) == 0) {
      debugf("or here..\n");
      return jbclass->jclass;
    }
    debugf("here..\n");
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
    if (jvm_strcmp(a->string, methodName) == 0)
      if (jvm_strcmp(b->string, methodType) == 0)
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
      case '[':
        break;
      case 'L':
        c++;
        /// run until we find semi-colon
        for (++x; typestr[x] != ';'; ++x);
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

void jvm_ScrubLocals(JVMLocal *locals, uint8 maxLocals) {
  int           y;

  for (y = 0; y < maxLocals; ++y) {
    if (locals[y].flags & JVM_STACK_ISOBJECTREF && locals[y].data != 0) {
      debugf("locals[%u].data:%x flags:%x\n", y, locals[y].data, locals[y].flags);
      ((JVMObject*)locals[y].data)->stackCnt--;
      debugf("SCRUB LOCALS ref:%lx refcnt:%i\n", locals[y].data, ((JVMObject*)locals[y].data)->stackCnt);
    }
  }
}
void jvm_ScrubStack(JVMStack *stack) {
  JVMLocal              result;
  while (jvm_StackMore(stack)) {
    jvm_StackPop(stack, &result);
    if (result.flags & JVM_STACK_ISOBJECTREF)
      if (result.data)
        ((JVMObject*)result.data)->stackCnt--;
  }
  debugf("done\n");
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

    if (jvm_strcmp((const char*)u8->string, className) == 0)
      return JVM_SUCCESS;
      /// if equals exit with true
      
    /// no super class exit with false
    if (jvm_strcmp(u8->string, "java/lang/Object") == 0)
      break;
    /// load class for specified super class
    ci = (JVMConstPoolClassInfo*)c->pool[c->superClass - 1];
    u8 = (JVMConstPoolUtf8*)c->pool[ci->nameIndex - 1];
    /// we got an error just not sure best way to handle it
    c = jvm_FindClassInBundle(bundle, u8->string);
    if (!c)
      break;
  }

  return JVM_ERROR_BADCAST;
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
        *class = jvm_FindClassInBundle(bundle, "java/lang/Array");
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
        return JVM_SUCCESS;
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

int jvm_MakeStaticFields(JVM *jvm, JVMBundle *bundle, JVMClass *class) {
  int                   x;
  int                   y;
  uint8                 *s;
  debugf("@@>in %lx\n", class);
  for (x = 0, y = 0; x < class->fieldCnt; ++x) {
    debugf("@@>HH1\n");
    if (class->fields[x].accessFlags & JVM_ACC_STATIC) {
      y++;
    }
    debugf("@@>HH2\n");
  }
  debugf("@@>NERE\n");
  if (!y)
    return y;
  class->sfieldCnt = y;
  class->sfields = (JVMObjectField*)jvm_malloc(sizeof(JVMObjectField) * y);
  for (x = 0, y = 0; x < class->fieldCnt; ++x) {
    if (class->fields[x].accessFlags & JVM_ACC_STATIC) {
      class->sfields[y].name = ((JVMConstPoolUtf8*)class->pool[class->fields[x].nameIndex - 1])->string;
      s = ((JVMConstPoolUtf8*)class->pool[class->fields[x].descIndex - 1])->string;
      jvm_FieldTypeStringToFlags(bundle, s, &class->sfields[y].jclass, &class->sfields[y].flags);
      debugf("static field\n");
      debugf("  desc:%s\n", s);
      debugf("  name:%s\n", class->sfields[y].name);
      debugf("  class:%x\n", class->sfields[y].jclass);
      class->sfields[y].aflags = 0;
      ++y;
    }
  }
  //
  if (jvm_FindMethodInClass(class, "<clinit>", "()V")) {
    debugf("@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    jvm_ExecuteObjectMethod(jvm, bundle, class, "<clinit>", "()V", 0, 0, 0);
  }
  debugf("@@>in\n");
  return y;
}

int jvm_MakeStaticFieldsOnBundle(JVM *jvm, JVMBundle *bundle) {
  JVMBundleClass        *bc;
  debugf("@@>nono\n");
  for (bc = bundle->first; bc != 0; bc = bc->next) {
    debugf("@@>yesyes1:%x %x\n", bc, bc->jclass);
    //jvm_MakeStaticFields(jvm, bundle, bc->jclass);
    debugf("@@>yesyes2\n");
  }
  debugf("@@>nono\n");
  //exit(-8);
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
  fields = (JVMObjectField*)jvm_malloc(sizeof(JVMObjectField) * fcnt);
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
      // I think this should be okay for primitive fields it has to be commented out.
      //if (jclass == 0) {
      //  debugf("jclass! %s %x\n", u8->string, fields[y].flags);
      //  jvm_exit(-9);
      //}
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

int jvm_PutField(JVMBundle *bundle, JVMObject *jobject, uint8 *fieldName, uintptr data, uint32 flags) {
  int           w;
  int           error;

  debugf("looking %x %u\n", jobject, jobject->fieldCnt);
  for (w = 0; w < jobject->fieldCnt; ++w) {
    debugf("checking %s with %s\n", jobject->_fields[w].name, fieldName);
    if (jvm_strcmp(jobject->_fields[w].name, fieldName) == 0) {
      // check that what we are placing here is a instance of the class psecified
      if (jobject->_fields[w].flags & JVM_STACK_ISOBJECTREF) {
        if (data) {
          if (jvm_IsInstanceOf(bundle, (JVMObject*)data, jvm_GetClassNameFromClass(jobject->_fields[w].jclass))) {
            return JVM_ERROR_BADCAST;
          }
        }
      }
      // end of check instance type8
      jobject->_fields[w].value = data;
      jobject->_fields[w].aflags = flags;
      return JVM_SUCCESS;
    }
    // end of match field name
  }
  debugf("no find field\n");
  // end of main loop
  return JVM_ERROR_MISSINGFIELD;
}

int jvm_GetField(JVMObject *jobject, uint8 *fieldName, JVMLocal *result) {
  int           w;

  for (w = 0; w < jobject->fieldCnt; ++w) {
    //debugf("%s==%s\n", fieldName, jobject->_fields[w].name);
    if (jvm_strcmp(jobject->_fields[w].name, fieldName) == 0) {
      result->data = jobject->_fields[w].value;
      result->flags = jobject->_fields[w].aflags;
      return JVM_SUCCESS;
    }
  }

  result->data = 0;
  result->flags = 0;
  return JVM_ERROR_MISSINGFIELD;
}

// helper function
int jvm_GetString(JVMObject *in, uint8 **out) {
  JVMLocal      result;
  int           error;
  uint32        flags;

  if (!in)
    return JVM_ERROR_INVALIDARG;
  
  error = jvm_GetField(in, "data", &result);
  //debugf("result.data:%x result.flags:%x\n", result.data, result.flags);
  if (error)
    return error;
  flags = JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF | JVM_STACK_ISBYTE;
  if (result.flags & flags == flags) {
    *out = (uint8*)(((JVMObject*)result.data)->_fields);
    return;
  }
  return JVM_ERROR_INVALIDARG;
}

int jvm_CreateString(JVM *jvm, JVMBundle *bundle, uint8 *string, uint16 szlen, JVMObject **out) {
  JVMObject                     *sobject;
  JVMObject                     *pobject;
  int                           error;
  
  error = jvm_CreateObject(jvm, bundle, "java/lang/String", &sobject);
  if (error)
    return error;
  error = jvm_CreatePrimArray(jvm, bundle, JVM_ATYPE_BYTE, szlen, &pobject, string);
  if (error)
    return error;
  error = jvm_PutField(bundle, sobject, "data", (uintptr)pobject, JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF | JVM_STACK_ISBYTE);
  JVM_OBJCOLRELEASE(sobject);
  if (error)
    return error;
  *out = sobject;
  return JVM_SUCCESS;
}

void jvm_MutexAquire(JVM_MUTEX *mutex) {
  while (__sync_lock_test_and_set(mutex, 1));
}

void jvm_MutexRelease(JVM_MUTEX *mutex) {
  *mutex = 0;
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
  jobject = (JVMObject*)jvm_malloc(sizeof(JVMObject));
  *out = jobject;
  if (!jobject)
    return JVM_ERROR_OUTOFMEMORY;
  memset(*out, 0, sizeof(JVMObject));
  jobject->class = jclass;
  jobject->stackCnt = 0;
  jobject->type = JVM_OBJTYPE_OBJECT;
  jobject->mutex = 0;
  JVM_OBJCOLHOLD(jobject);
  // link us into global object chain
  jvm_MutexAquire(&jvm->mutex);
  jobject->next = jvm->objects;
  jvm->objects = jobject;
  jvm_MutexRelease(&jvm->mutex);
  // go through and create fields
  jvm_MakeObjectFields(jvm, bundle, jobject);
  // execute init method
  locals[0].data = (uint64)jobject;
  locals[0].flags = JVM_STACK_ISOBJECTREF;
  // call default constructor (no arguments)
  debugf("##>%x\n", jobject);
  // why the hell am i calling this here? am i supposed to for some
  // weird reason that i forgot about, a java.lang.Character was created
  // with new opcode.. then it calls the constructor later.. okay if stuff
  // goes crazy this is where it happened
  // --kmcguire 7/23/12
  //return jvm_ExecuteObjectMethod(jvm, bundle, jclass, "<init>", "()V", &locals[0], 1, &result);
  return JVM_SUCCESS;
}

void jvm_AddClassToBundle(JVMBundle *jbundle, JVMClass *jclass) {
  JVMBundleClass                *jbclass;

  debugf("@@>jclass:%x\n", jclass);
  jbclass = (JVMBundleClass*)jvm_malloc(sizeof(JVMBundleClass));
  jbclass->jclass = jclass;

  jbclass->next = jbundle->first;
  jbundle->first = jbclass;
  return;
}

#define JVM_HAND_LMAC(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))


/// garabage collector node structure
typedef struct _JVMCollect {
  struct _JVMCollect    *next;
  JVMObject             *object;        
} JVMCollect;

int jvm_Collect(JVM *jvm) {
  uint16                cmark;  
  JVMCollect            *ra, *ca, *_ca;
  JVMCollect            *rb, *cb;
  JVMObject             *co, *_co;
  JVMObjectField        *cof;
  JVMObject             **caf;
  JVMObject             *r;
  int                   x;

  ra = 0;
  rb = 0;

  // using lower 4-bits for cmark
  // using upper 4-bits for flags
  jvm->cmark = (jvm->cmark + 1) & 0xf;
  cmark = jvm->cmark & 0xf;
  
  debugf("garbage collector cmark is %x\n", cmark);
  
  // add all objects to ra
  jvm_MutexAquire(&jvm->mutex);
  for (co = jvm->objects; co != 0; co = co->next) {
    if ((co->stackCnt > 0) || (co->cmark & JVM_OBJECT_CMARK_FLAG_NOCOLLECT)) {
      ca = (JVMCollect*)jvm_malloc(sizeof(JVMCollect));
      ca->object = co;
      ca->next = ra;
      ra = ca;
      debugf("obj %x %u\n", co, co->stackCnt);
    }
  }
  //exit(-4);

  while (ra != 0) {
    for (ca = ra; ca != 0; ca = ca->next) {
      // do not do objects already done
      if (ca->object->cmark & 0xf == cmark)
        continue;
      // create collects from fields pointing to other objects
      debugf("pp:%x\n", cof);
      switch (ca->object->type)
      {
        case JVM_OBJTYPE_PARRAY:
          // stores primitive types
          break;
        case JVM_OBJTYPE_OBJECT:
          cof = ca->object->_fields;
          for (x = 0; x < ca->object->fieldCnt; ++x) {
            // add collect to rb
            debugf("rr x:%u\n", x);
            // null, nothing to do, continue onward
            if (!cof[x].value)
              continue;
            // some fields may be primitive, we are only
            // interested in object
            if (cof[x].aflags & JVM_STACK_ISOBJECTREF) {
              if (((JVMObject*)cof[x].value)->cmark == cmark)
                continue;
              // it will be marked when reading from ra chain
              cb = (JVMCollect*)jvm_malloc(sizeof(JVMCollect));
              cb->object = (JVMObject*)cof[x].value;
              cb->next = rb;
              rb = cb;
            }
            debugf("-rr\n");
          }
          break;
        case JVM_OBJTYPE_OARRAY:
          caf = (JVMObject**)ca->object->fields;
          for (x = 0; x < ca->object->fieldCnt; ++x) {
            if (caf[x]->cmark & 0xf == cmark)
              continue;
            cb = (JVMCollect*)jvm_malloc(sizeof(JVMCollect));
            cb->object = (JVMObject*)caf[x];
            cb->next = rb;
            rb = cb;
          }
          break;
      }
      debugf("here ca:%x\n", ca);
      // mark this object
      ca->object->cmark = cmark;
    }
    debugf("apple\n");
    // clear ra and reset it to empty
    for (ca = ra; ca != 0; ca = _ca) {
      _ca = ca->next;
      jvm_free(ca);
    }
    // switch ra and rb
    ra = rb;
    rb = 0;
  }

  // new chain head
  r = 0;
  for (co = jvm->objects; co != 0; co = _co) {
    _co = co->next;
    // stackCnt must be zero (object not on any stack or any local slot)
    // cmark lower 4-bits must not equal current jvm cmark (meaning unreachable as reference from anything)
    // cmark must not have NOCOLLECT FLAG set (upper 4-bits)
    if ((co->stackCnt == 0) && ((co->cmark & 0xf) != cmark) && (!(co->cmark & JVM_OBJECT_CMARK_FLAG_NOCOLLECT))) {
      // free and forget about these
      debugf("FREE type:%x obj:%x cmark:%u stackCnt:%u class:%s\n", co->type, co, co->cmark, co->stackCnt, jvm_GetClassNameFromClass(co->class));
      jvm_free(co);
    } else {
      debugf("KEEP type:%x obj:%x cmark:%u stackCnt:%u class:%s\n", co->type, co, co->cmark, co->stackCnt, jvm_GetClassNameFromClass(co->class));
      // relink keepers into new chain
      co->next = r;
      r = co;
    }
  }

  // set new chain in old chain's place
  // items in the old chain are actually now invalid
  // memory.. so got to make sure to do this
  jvm->objects = r;

  jvm_MutexRelease(&jvm->mutex);
  return 1;
}
