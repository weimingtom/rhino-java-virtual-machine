#include "rjvm.h"
#include "exec.h"

void jvm_LocalPut(JVMLocal *locals, uint32 ndx, uintptr data, uint32 flags) {
  if (locals[ndx].flags & JVM_STACK_ISOBJECTREF) {
    debugf("gggg locals[%u]:%x\n", ndx, locals[ndx].data);
    if (locals[ndx].data) {
      ((JVMObject*)locals[ndx].data)->stackCnt--;
    }
  }
  locals[ndx].data = data;
  locals[ndx].flags = flags;
  if (flags & JVM_STACK_ISOBJECTREF)
    if (data)
      ((JVMObject*)data)->stackCnt++;
}
/// create object array
int jvm_CreateObjectArray(JVM *jvm, JVMBundle *bundle, uint8 *className, uint32 size, JVMObject **_object) {
  JVMObject     *_jobject;

  _jobject = (JVMObject*)jvm_malloc(sizeof(JVMObject));
  _jobject->next = jvm->objects;
  _jobject->type = JVM_OBJTYPE_OARRAY;
  jvm->objects = _jobject;
  _jobject->class = jvm_FindClassInBundle(bundle, className);
  _jobject->stackCnt = 0;
  _jobject->fields = (uint64*)jvm_malloc(sizeof(JVMObject*) * size);
  _jobject->fieldCnt = size;

  *_object = _jobject;
  return JVM_SUCCESS;
  //jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
}
/// create primitive array
int jvm_CreatePrimArray(JVM *jvm, JVMBundle *bundle, uint8 type, uint32 cnt, JVMObject **jobject) {
  JVMObject             *_jobject;

  // JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF
  _jobject = (JVMObject*)jvm_malloc(sizeof(JVMObject));
  _jobject->next = jvm->objects;
  _jobject->type = JVM_OBJTYPE_PARRAY;
  jvm->objects = _jobject;
  _jobject->class = jvm_FindClassInBundle(bundle, "java/lang/Array");
  if (!_jobject->class) {
    debugf("whoa.. newarray but java/lang/Array not in bundle!\n");
    jvm_exit(-99);
  }
  _jobject->fields = 0;
  _jobject->stackCnt = 0;
  _jobject->cmark = 0;
  switch(type) {
    case JVM_ATYPE_LONG:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint64) * cnt);
      break;
    case JVM_ATYPE_INT:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint32) * cnt);
      break;
    case JVM_ATYPE_CHAR:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint8) * cnt);
      break;
    case JVM_ATYPE_BYTE:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint8) * cnt);
      break;
    case JVM_ATYPE_FLOAT:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint32) * cnt);
      break;
    case JVM_ATYPE_DOUBLE:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint64) * cnt);
      break;
    case JVM_ATYPE_BOOL:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint8) * cnt);
      break;
    case JVM_ATYPE_SHORT:
      _jobject->fields = (uint64*)jvm_malloc(sizeof(uint16) * cnt);
      break;
  }
  _jobject->fieldCnt = (uintptr)cnt;
  //jvm_StackPush(&stack, (uint64)_jobject, (y << 4) | JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
  //jvm_DebugStack(&stack);
  *jobject = _jobject;
  return JVM_SUCCESS;
}

int jvm_ExecuteObjectMethod(JVM *jvm, JVMBundle *bundle, JVMClass *jclass,
                         const char *methodName, const char *methodType,
                         JVMLocal *_locals, uint8 localCnt, JVMLocal *_result) {
  /// this is mostly long lived things so becareful
  /// when using one to do some work it might be
  /// holding a long term value
  JVMMethod                     *method;
  JVMConstPoolUtf8              *a;
  int                           x;
  JVMLocal                      *locals;
  uint32                        codesz;
  uint8                         *code;
  uint8                         opcode;
  JVMStack                      stack;
  int32                         error;

  /// mostly temporary stuff.. used in small blocks
  /// check around but should be safe to use here
  /// and there for small scopes
  int32                        *map;            // switchtable
  int32                         _error;
  JVMLocal                      result;
  JVMLocal                      result2;
  intptr                        y, w, z, g;     // all
  uint32                        k;              // switchtable
  int32                         v;              // switchtable
  int32                         eresult;
  JVMClass                      *_jclass;
  JVMObject                     *__jobject;
  JVMObject                     *_jobject;
  JVMMethod                     *_method;
  JVMConstPoolMethodRef         *b;
  JVMConstPoolClassInfo         *c;
  JVMConstPoolNameAndType       *d;
  JVMConstPoolFieldRef          *f;
  JVMConstPoolString            *s;
  int                           argcnt;
  uint8                         *mclass;
  uint8                         *mmethod;
  uint8                         *mtype;
  uint8                         *tmp;

  //g_dbg_ec++;
  //if (g_dbg_ec == 3) {
  //  jvm_printf(("g_dbg_ec=%u\n", g_dbg_ec);
  //  jvm_exit(-9);
  //}

  debugf("executing %s\n", methodName);

  /// find method specifiee
  debugf("classname:%s\n", jvm_GetClassNameFromClass(jclass));
  method = jvm_FindMethodInClass(jclass, methodName, methodType);
  if (!method) {
    debugf("JVM_ERROR_METHODNOTFOUND\n");
    return JVM_ERROR_METHODNOTFOUND;
  }

  /// do we have code? hopefully...
  if (!method->code) {
    debugf("JVM_ERROR_NOCODE\n");
    return JVM_ERROR_NOCODE;
  }

  code = method->code->code;
  codesz = method->code->codeLength;

  // weird bug.. i need to add a little onto the stack because
  // it likes to run over the end a little.. maybe this is just
  // a bandaid and a deeper problem lies to be found
  jvm_StackInit(&stack, method->code->maxStack + 4);
  error = 0;
  
  debugf("method has code(%lx) of length %u\n", code, codesz);
  /// -----------------------------------------------------
  /// i think there is a way to determine how much local
  /// variable space is needed... but for now this will work
  /// -----------------------------------------------------
  /// 255 should be maximum local addressable
  locals = (JVMLocal*)jvm_malloc(sizeof(JVMLocal) * method->code->maxLocals);
  /// copy provided arguments into locals
  debugf("----->maxLocals:%x\n", method->code->maxLocals);
  // more max locals can be specified than actual
  // number of locals passed in
  for (x = 0; x < method->code->maxLocals; ++x) {
    locals[x].data = 0;
    locals[x].flags = 0;
  }
  debugf("localCnt:%u\n", localCnt);
  for (x = 0; x < localCnt; ++x) {
    locals[x].data = _locals[x].data;
    locals[x].flags = _locals[x].flags;
    debugf("$$data:%u flags:%u\n", _locals[x].data, _locals[x].flags);
    if (locals[x].flags & JVM_STACK_ISOBJECTREF)
      if (locals[x].data)
        ((JVMObject*)locals[x].data)->stackCnt++;
  }

  debugf("execute code\n");
  /// execute code
  for (x = 0; x < codesz;) {
    opcode = code[x];
    debugf("---dumping objects---\n");
    for (_jobject = jvm->objects; _jobject != 0; _jobject = _jobject->next) {
      debugf("jobject:%x\tstackCnt:%i\tclassName:%s\n", _jobject, _jobject->stackCnt, jvm_GetClassNameFromClass(_jobject->class));
    }
    debugf("\e[7;31mopcode(%u/%u):%x className:%s methodName:%s\e[m\n", x, codesz, opcode, jvm_GetClassNameFromClass(jclass), methodName);

    for (_jobject = jvm->objects; _jobject != 0; _jobject = _jobject->next) {
      if (_jobject->cmark == 12032) {
        debugf("object:%x object->cmark:%x set\n", _jobject, 12032);
        exit(-5);
      }
    }
    
    jvm_DebugStack(&stack);
    //fgetc(stdin);
    switch (opcode) {
      /// nop: no operation
      case 0:
        x += 2;
        break;
      /// ldc: push a constant #index from a constant pool (string, int, or float) onto the stack
      case 0x12:
        debugf("LDC\n");
        y = code[x+1];
        /// determine what this index refers too
        switch (jclass->pool[y - 1]->type) {
          case TAG_STRING:
            // navigate constant pool
            a = (JVMConstPoolUtf8*)jclass->pool[((JVMConstPoolString*)jclass->pool[y - 1])->stringIndex - 1];
            // get string length in 'w'
            for (w = 0; a->string[w] != 0; ++w);
            // create java/lang/String
            _jobject = 0;
            if(jvm_CreateObject(jvm, bundle, "java/lang/String", &_jobject)) {
              error = JVM_ERROR_CLASSNOTFOUND;
              break;
            }
            // create byte array to hold string
            /// todo: ref our byte[] to String
            /// todo: also do for putfield and getfield opcodes
            jvm_CreatePrimArray(jvm, bundle, JVM_ATYPE_BYTE, w, &__jobject);
            __jobject->stackCnt = 1;
            debugf("_jobject->_fields:%x\n", _jobject->_fields);
            for (w = 0; w < _jobject->fieldCnt; ++w) {
              debugf("%s.%s.%u\n", _jobject->_fields[w].name, "data", w);
              if (jvm_strcmp(_jobject->_fields[w].name, "data") == 0) {
                _jobject->_fields[w].value = (uintptr)__jobject;
                _jobject->_fields[w].aflags = JVM_STACK_ISARRAYREF |
                                              JVM_STACK_ISOBJECTREF |
                                              JVM_STACK_ISBYTE;
                break;
              }
            }
            // copy string into primitive byte array
            for (w = 0; a->string[w] != 0; ++w)
              ((uint8*)__jobject->fields)[w] = a->string[w];
            // push onto stack the String object
            jvm_StackPush(&stack, _jobject, JVM_STACK_ISOBJECTREF);
            jvm_DebugStack(&stack);
            //debugf("STOP w:%u\n", w);
            //jvm_exit(-3);
            break;
          case TAG_INTEGER:
            jvm_StackPush(&stack, (intptr)((JVMConstPoolInteger*)jclass->pool[y - 1])->value, JVM_STACK_ISINT);
            break;
          case TAG_FLOAT:
            debugf("TAG_FLOAT not implemented!\n");
            jvm_exit(-9);
            break;
        }
        x += 2;
        break;
      /// ldc_w:
      /// ldc2_w:
      /// ifnull: if value is null branch
      case 0xc6:
      y = (int16)(code[x+1] << 8 | code[x+2]);
      jvm_StackPop(&stack, &result);
      if (result.flags != JVM_STACK_ISNULL) {
        x += 3;
      } else {
        x += y;
      }
      break;
      /// ifnonnull: if value is not null branch at instruction
      case 0xc7:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result);
        if (result.flags != JVM_STACK_ISNULL) {
          x += y;
        } else {
          x += 3;
        }
        break;
      /// aconst_null: push null reference onto stack
      case 0x01:
        jvm_StackPush(&stack, 0, JVM_STACK_ISNULL);
        x += 1;
        break;
      /// checkcast
      case 0xc0:
        y = code[x+1] << 8 | code[x+2];
        c = (JVMConstPoolClassInfo*)jclass->pool[y - 1];
        a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
        mclass = a->string;
        
        /// more like a stack peek..
        jvm_StackPop(&stack, &result);
        jvm_StackPush(&stack, result.data, result.flags);

        /// convert type string into flags
        if (mclass[0] == '[') {
          // this is being casted to a primitive array
          jvm_FieldTypeStringToFlags(bundle, a->string, &_jclass, &w);
        
          if (w != result.flags) {
            debugf("bad primitive array cast\n");
            error = JVM_ERROR_BADCAST;
            break;
          }
          x += 3;
          break;
        }
        // this is a object cast to another object by classname
        // so mclass must exist as this class or a super of it
        _jobject = (JVMObject*)result.data;
        /// is objref the type described by Y (or can be)
        if (jvm_IsInstanceOf(bundle, _jobject, mclass))
        {
          debugf("bad cast to %s\n", mclass);
          debugf("i am here\n");
          jvm_exit(-9);
          error = JVM_ERROR_BADCAST;
          break;
        }
        x += 3;
        break;
      /// iinc: increment local variable #index by signed byte const
      case 0x84:
        y = code[x+1];
        w = (int8)code[x+2];
        locals[y].data += (int8)w;
        x += 3;
        break;
      /// if_icmpgt
      case 0xa3:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result2);
        jvm_StackPop(&stack, &result);
        if ((int64)result.data > (int64)result2.data) {
          x += y;
          break;
        }
        x += 3;
        break;        
      /// if_icmplt
      case 0xa1:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result2);
        jvm_StackPop(&stack, &result);
        if ((int64)result.data < (int64)result2.data) {
          x += y;
          break;
        }
        x += 3;
        break;
      /// if_icmpeq
      case 0x9f:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result2);
        jvm_StackPop(&stack, &result);
        if ((int64)result.data == (int64)result2.data) {
          x += y;
          break;
        }
        x += 3;
        break;        
      /// if_icmpne
      case 0xa0:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result2);
        jvm_StackPop(&stack, &result);
        if ((int64)result.data != (int64)result2.data) {
          x += y;
          break;
        }
        x += 3;
        break;        
      /// if_icmpge
      case 0xa2:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result2);
        jvm_StackPop(&stack, &result);
        if ((int64)result.data >= (int64)result2.data) {
          x += y;
          break;
        }
        x += 3;
        break;
      /// if_icmple
      case 0xa4:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result2);
        jvm_StackPop(&stack, &result);
        if ((int64)result.data <= (int64)result2.data) {
          x += y;
          break;
        }
        x += 3;
        break;
      /// athrow
      case 0xbf:
        error = JVM_ERROR_EXCEPTION;
        break;
      /// dup: duplicate item on top of stack
      case 0x59:
        jvm_StackPop(&stack, &result);
        /// now there are two references instead of one
        jvm_StackPush(&stack, result.data, result.flags);
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      /// new: create new instance of object
      case 0xbb:
        y = code[x+1] << 8 | code[x+2];
        // classinfo
        c = (JVMConstPoolClassInfo*)jclass->pool[y - 1];
        a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
        error = jvm_CreateObject(jvm, bundle, a->string, &_jobject);
        // jump out and create exception if we can..
        if (error < 0)
          break;
        _jobject->type = JVM_OBJTYPE_OBJECT;
        _jobject->stackCnt = 0;
        // place object instance onto stack
        jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISOBJECTREF);
        x += 3;
        break;
      /// goto
      case 0xa7:
        y = code[x+1] << 8 | code[x+2];
        x = x + (int16)y;
        break;
      /// ifeq
      case 0x99:
        y = code[x+1] << 8 | code[x+2];
        jvm_StackPop(&stack, &result);
        if (stack.data == 0) {
          x = x + y;
          break;
        }
        x += 3;
        break;        
      /// ifne
      case 0x9a:
        y = code[x+1] << 8 | code[x+2];
        jvm_StackPop(&stack, &result);
        if (stack.data != 0) {
          x = x + y;
          break;
        }
        x += 3;
        break;
      /// i2b
      case 0x91:
        jvm_StackPop(&stack, &result);
        result.data = (uint8)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      /// i2c
      case 0x92:
        jvm_StackPop(&stack, &result);
        result.data = (uint8)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      /// i2d
      case 0x87:
        jvm_StackPop(&stack, &result);
        ((double*)result.data)[0] = (double)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      /// i2f
      case 0x86:
        jvm_StackPop(&stack, &result);
        ((float*)result.data)[0] = (float)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      /// i2l
      case 0x85:
        jvm_StackPop(&stack, &result);
        result.data = (int64)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      /// i2s
      case 0x93:
        jvm_StackPop(&stack, &result);
        result.data = (int16)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      /// iconst_m1
      case 0x02:
        jvm_StackPush(&stack, -1, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iconst_0
      case 0x03:
        jvm_StackPush(&stack, 0, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iconst_1
      case 0x04:
        jvm_StackPush(&stack, 1, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iconst_2
      case 0x05:
        jvm_StackPush(&stack, 2, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iconst_3
      case 0x06:
        jvm_StackPush(&stack, 3, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iconst_4
      case 0x07:
        jvm_StackPush(&stack, 4, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iconst_5
      case 0x08:
        jvm_StackPush(&stack, 5, JVM_STACK_ISINT);
        x += 1;
        break;
      /// istore
      case 0x36:
        y = code[x+1];
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, y, result.data, JVM_STACK_ISINT);
        x += 2;
        break;
      /// istore_0
      case 0x3b:
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, 0, result.data, JVM_STACK_ISINT);
        x += 1;
        break;
      /// istore_1
      case 0x3c:
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, 1, result.data, JVM_STACK_ISINT);
        x += 1;
        break;
      /// istore_2
      case 0x3d:
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, 2, result.data, JVM_STACK_ISINT);
        x += 1;
        break;
      /// istore_3
      case 0x3e:
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, 3, result.data, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iload
      case 0x15:
        y = code[x+1];
        jvm_StackPush(&stack, locals[y].data, locals[y].flags);
        x += 2;
        break;
      /// iload_0
      case 0x1a:
        jvm_StackPush(&stack, locals[0].data, locals[0].flags);
        x += 1;
        break;
      /// iload_1
      case 0x1b:
        jvm_StackPush(&stack, locals[1].data, locals[1].flags);
        x += 1;
        break;
      /// iload_2
      case 0x1c:
        jvm_StackPush(&stack, locals[2].data, locals[2].flags);
        x += 1;
        break;
      /// iload_3
      case 0x1d:
        jvm_StackPush(&stack, locals[3].data, locals[3].flags);
        x += 1;
        break;
      /// astore
      case 0x3a:
        y = code[x+1];
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, y, result.data, result.flags);
        x += 2;
        break;
      /// astore_0
      case 0x4b:
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, 0, result.data, result.flags);
        x += 1;
        break;
      /// astore_1
      case 0x4c:
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, 1, result.data, result.flags);
        x += 1;
        break;
      /// astore_2
      case 0x4d:
        jvm_StackPop(&stack, &result);
        debugf("eee %u %x data:%x flags:%x\n", method->code->maxLocals, locals, result.data, result.flags);
        jvm_LocalPut(locals, 2, result.data, result.flags);
        debugf("fff\n");
        x += 1;
        break;
      /// astore_3
      case 0x4e:
        jvm_StackPop(&stack, &result);
        jvm_LocalPut(locals, 3, result.data, result.flags);
        x += 1;
        break;
      /// ineg: interger negate
      case 0x74:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPush(&stack, ~y, JVM_STACK_ISINT);
        x += 1;
        break;        
      /// ior:
      case 0x80:
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPush(&stack, y | w, JVM_STACK_ISINT);
        x += 1;
        break;      
      /// iushr: logical shift right on int
      case 0x7c:
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPush(&stack, ~(~0 >> w) | (y >> w), JVM_STACK_ISINT);
        x += 1;
        break;        
      /// irem: logical int remainder
      case 0x70:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, (int32)w % (int32)y, JVM_STACK_ISINT);
        x += 1;
        break;
      /// xor
      case 0x82:
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPush(&stack, y ^ w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// ishl
      case 0x78:
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPush(&stack, y << w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// ishr
      case 0x7a:
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPush(&stack, y >> w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// isub
      case 0x64:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, (int32)w - (int32)y, JVM_STACK_ISINT);
        x += 1;
        break;
      /// iand: bitwise and on two ints
      case 0x7e:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y & w, JVM_STACK_ISINT);
        x += 1;
        break;        
      /// idiv: divide two ints
      case 0x6c:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, (int32)w / (int32)y, JVM_STACK_ISINT);
        x += 1;
        break;
      /// imul: multiply two ints
      case 0x68:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, (int32)y * (int32)w, JVM_STACK_ISINT);
        x += 1;
        break;        
      /// iadd: add two ints
      case 0x60:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result2);
        w = result2.data;
        jvm_StackPush(&stack, (int32)y + (int32)w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// ----------------------------------------------
      /// ARRAY LOAD CODE
      /// ----------------------------------------------
      /// caload: load char from an array
      case 0x34:
      /// daload: load double from an array
      case 0x31:
      /// faload: load float from an array
      case 0x30:
      /// iaload: load an int from an array
      case 0x2e:
      /// laload: load long from array
      case 0x2f:
      /// saload: load short from array
      case 0x35:
      /// baload: load byte/boolean from arraylength
      case 0x33:
        /// index
        jvm_StackPop(&stack, &result);
        w = result.data;
        /// array ref
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
        if (!_jobject) {
          error = JVM_ERROR_NULLOBJREF;
          break;
        }

        /// is this an object and array?
        if (result.flags & (JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF) !=
                           (JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF) ) {
          /// is it not so throw an error
          error = JVM_ERROR_NOTOBJORARRAY;
          break;
        }
        
        if (w >= _jobject->fieldCnt) {
          /// error, past end of array..
          error = JVM_ERROR_ARRAYOUTOFBOUNDS;
          break;
        }
        switch(opcode) {
          /// caload: load char from an array
          case 0x34:
            jvm_StackPush(&stack, ((int8*)_jobject->fields)[w], JVM_STACK_ISCHAR);
            break;
          /// daload: load double from an array
          case 0x31:
            jvm_StackPush(&stack, ((double*)_jobject->fields)[w], JVM_STACK_ISDOUBLE);
            break;
          /// faload: load float from an array
          case 0x30:
            jvm_StackPush(&stack, ((float*)_jobject->fields)[w], JVM_STACK_ISFLOAT);
            break;
          /// iaload: load an int from an array
          case 0x2e:
            jvm_StackPush(&stack, ((uint32*)_jobject->fields)[w], JVM_STACK_ISINT);
            break;
          /// laload: load long from array
          case 0x2f:
            jvm_StackPush(&stack, ((int64*)_jobject->fields)[w], JVM_STACK_ISLONG);
            break;
          /// saload: load short from array
          case 0x35:
            jvm_StackPush(&stack, ((int16*)_jobject->fields)[w], JVM_STACK_ISSHORT);
            break;
          /// baload: load byte/boolean from array
          case 0x33:
            debugf("arrived!\n");
            jvm_StackPush(&stack, ((uint8*)_jobject->fields)[w], JVM_STACK_ISBYTE);
            debugf("arrived!\n");
            break;
        }
        x += 1;
        break;
      /// ----------------------------------------------
      /// ARRAY STORE CODE
      /// ----------------------------------------------
      /// bastore: store byte/boolean in array
      case 0x54:
      /// sastore: store short into array
      case 0x56:
      /// lastore: store long into an array
      case 0x50:
      /// dastore: store double into an array
      case 0x52:        
      /// fastore: store float into an array
      case 0x51:
      /// castore: store char into an array
      case 0x55:
      /// iastore: store an int into an array
      case 0x4f:
        /// value
        jvm_StackPop(&stack, &result);
        y = result.data;
        /// index
        jvm_StackPop(&stack, &result);
        w = result.data;
        /// array ref
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
        if (!_jobject) {
          error = JVM_ERROR_NULLOBJREF;
          break;
        }
        /// is this an object and array?
        if (result.flags & (JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF) !=
                           (JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF) ) {
          /// is it not so throw an error
          error = JVM_ERROR_NOTOBJORARRAY;
          break;
        }
        if (w >= (uint64)_jobject->fieldCnt) {
          /// error, past end of array..
          error = JVM_ERROR_ARRAYOUTOFBOUNDS;
          break;
        }
        switch (opcode) {
          /// bastore: store byte/boolean in array
          case 0x54:
            ((uint8*)_jobject->fields)[w] = y;
            break;
          /// sastore: store short into array
          case 0x56:
            ((int16*)_jobject->fields)[w] = y;
            break;
          /// lastore: store long into an array
          case 0x50:
            ((int64*)_jobject->fields)[w] = y;
            break;
          /// dastore: store double into an array
          case 0x52:
            ((double*)_jobject->fields)[w] = y;
            break;
          /// fastore: store float into an array
          case 0x51:
            ((float*)_jobject->fields)[w] = y;
            break;
          /// castore: store char into an array
          case 0x55:
            ((int64*)_jobject->fields)[w] = y;
            break;
          /// iastore: store an int into an array
          case 0x4f:
            ((int32*)_jobject->fields)[w] = y;
            break;
        }        
        //if (opcode == 0x2e)
        //  jvm_StackPush(&stack, ((uint64*)_jobject->fields)[w], JVM_STACK_ISINT);
        x += 1;
        break;
      /// arraylength
      case 0xbe:
        jvm_DebugStack(&stack);
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
        if (!_jobject) {
          error = JVM_ERROR_NULLOBJREF;
          break;
        }
        debugf("before push\n");
        jvm_StackPush(&stack, (uint64)_jobject->fieldCnt, JVM_STACK_ISINT);
        x += 1;
        break;
      /// aastore: store ref into ref array
      case 0x53:
        jvm_DebugStack(&stack);
        jvm_StackPop(&stack, &result);
        __jobject = (JVMObject*)result.data;
        // we do not want anything but an object reference
        if (!(result.flags & JVM_STACK_ISOBJECTREF) && !(result.flags & JVM_STACK_ISNULL)) {
          error = JVM_ERROR_NOTOBJORARRAY;
          break;
        }

        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
        // make sure the array is an actual arrayref
        // primitive arrays are the same but with an
        // extra type flag
        g = JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF;
        if (result.flags != g) {
          error = JVM_ERROR_NOTOBJORARRAY;
          break;
        }

        if (__jobject != 0) {
          // check that the type we are trying to store
          // is the same or derived from
          if (jvm_IsInstanceOf(bundle, __jobject, jvm_GetClassNameFromClass(_jobject->class))) {
            error = JVM_ERROR_WASNOTINSTANCEOF;
            break;
          }
        }
       
        if (w >= _jobject->fieldCnt) {
          // error, past end of array..
          error = JVM_ERROR_ARRAYOUTOFBOUNDS;
          break;
        }

        ((JVMObject**)_jobject->fields)[w] = __jobject;
        
        x += 1;
        jvm_DebugStack(&stack);
        break;
      /// aaload: load onto stack from ref array
      case 0x32:
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
        // make sure the array is an actual arrayref
        // primitive arrays are the same but with an
        // extra type flag
        g = JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF;
        if (result.flags != g) {
          error = JVM_ERROR_NOTOBJORARRAY;
          break;
        }
        // we are inside the bounds
        if (w >= _jobject->fieldCnt) {
          /// error, past end of array..
          error = JVM_ERROR_ARRAYOUTOFBOUNDS;
          break;
        }

        __jobject = ((JVMObject**)_jobject->fields)[w];
        if (__jobject == 0) {
          jvm_StackPush(&stack, 0, JVM_STACK_ISNULL);
        } else {
          jvm_StackPush(&stack, (uint64)__jobject, JVM_STACK_ISOBJECTREF);
        }
        x += 1;
        break;
      /// anewarray: create array of references by type specified
      case 0xbd:
        y = code[x+1] << 8 | code[x+2];
        _jobject = (JVMObject*)jvm_malloc(sizeof(JVMObject));
        _jobject->next = jvm->objects;
        _jobject->type = JVM_OBJTYPE_OARRAY;
        jvm->objects = _jobject;
        /// allows us to know if this is a ref array for a type
        /// or if it is a primitive array=0 while obj array!=0
        c = (JVMConstPoolClassInfo*)jclass->pool[y - 1];
        a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
        _jobject->class = jvm_FindClassInBundle(bundle, a->string);
        _jobject->stackCnt = 0;
        jvm_StackPop(&stack, &result);
        argcnt = result.data;
        _jobject->fields = (uint64*)jvm_malloc(sizeof(JVMObject*) * argcnt);
        _jobject->fieldCnt = argcnt;
        jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
        debugf("##> anewarray %x\n", _jobject);
        x += 3;
        break;
      /// newarray
      case 0xbc:
        y = code[x+1];
        jvm_StackPop(&stack, &result);
        jvm_CreatePrimArray(jvm, bundle, y, result.data, &_jobject);
        debugf("##> primarray %x\n", _jobject);
        jvm_StackPush(&stack, (uintptr)_jobject, (y << 4) | JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
        x += 2;
        break;
      /// sipush: push a short onto the stack
      case 0x11:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPush(&stack, y, JVM_STACK_ISSHORT);
        x += 3;
        break;
      /// bipush: push a byte onto the stack as an integer
      case 0x10:
        y = (int8)code[x+1];
        jvm_StackPush(&stack, y, JVM_STACK_ISINT);
        x += 2;
        break;
      /// pop: discard top value on stack
      case 0x57:
        jvm_StackPop(&stack, &result);
        x += 1;
        break;
      /// aload: load a reference onto the stack from local variable 'y'
      case 0x19:
        y = code[x+1];
        jvm_StackPush(&stack, locals[y].data, locals[y].flags);
        x += 2;
        break;
      /// aload_0: load a reference onto the stack from local variable 0
      case 0x2a:
        jvm_StackPush(&stack, locals[0].data, locals[0].flags);
        x += 1;
        break;
      /// aload_1
      case 0x2b:
        jvm_StackPush(&stack, locals[1].data, locals[1].flags);
        x += 1;
        break;
      /// aload_2
      case 0x2c:
        jvm_StackPush(&stack, locals[2].data, locals[2].flags);
        x += 1;
        break;
      /// aload_3
      case 0x2d:
        jvm_StackPush(&stack, locals[3].data, locals[3].flags);
        x += 1;
        break;
      /// getstatic
      case 0xb2:
        // name index into const pool table
        y = code[x+1] << 8 | code[x+2];

        f = (JVMConstPoolFieldRef*)jclass->pool[y - 1];
        d = (JVMConstPoolNameAndType*)jclass->pool[f->nameAndTypeIndex - 1];
        a = (JVMConstPoolUtf8*)jclass->pool[d->nameIndex - 1];

        //
        error = JVM_ERROR_MISSINGFIELD;
        for (w = 0; w < jclass->sfieldCnt; ++w) {
          debugf("looking for field %s have %s\n", a->string, jclass->sfields[w].name);
          if (jvm_strcmp(jclass->sfields[w].name, a->string) == 0) {
            // push onto the stack
            debugf("jclass->sfields[w].value:%li\n", jclass->sfields[w].value);
            jvm_StackPush(&stack, jclass->sfields[w].value, jclass->sfields[w].aflags);
            error = JVM_SUCCESS;
            break;
          }
        }
        if (error > 0) {
          debugf("field not found\n");
          jvm_exit(-7);
          break;
        }
        x += 3;
        break;
      /// getfield
      case 0xb4:
        // name index into const pool table
        y = code[x+1] << 8 | code[x+2];
        // object
        jvm_StackPop(&stack, &result);
        // may add type check here one day
        _jobject = (JVMObject*)result.data;
        _jclass = _jobject->class;

        debugf("_jobject.name:%s\n", jvm_GetClassNameFromClass(_jobject->class));

        if (_jclass->pool[y - 1]->type == 1) {
          a = (JVMConstPoolUtf8*)_jclass->pool[y - 1];
          debugf("fieldname:%s\n", a->string);
        } else {
          f = (JVMConstPoolFieldRef*)_jclass->pool[y - 1];
          debugf("mmm:%u\n", _jclass->pool[y - 1]->type);
          d = (JVMConstPoolNameAndType*)_jclass->pool[f->nameAndTypeIndex - 1];
          a = (JVMConstPoolUtf8*)_jclass->pool[d->nameIndex - 1];
        }

        error = jvm_GetField(_jobject, a->string, &result);
        if (error != JVM_SUCCESS)
          break;
        
        jvm_StackPush(&stack, result.data, result.flags);
        
        
        debugf("fieldCnt %u\n", _jobject->fieldCnt);
        error = JVM_ERROR_MISSINGFIELD;;
        for (w = 0; w < _jobject->fieldCnt; ++w) {
          debugf("##>%x looking for field %s have %s\n", _jobject, a->string, _jobject->_fields[w].name);
          if (jvm_strcmp(_jobject->_fields[w].name, a->string) == 0) {
            // push onto the stack
            jvm_StackPush(&stack, _jobject->_fields[w].value, _jobject->_fields[w].aflags);
            error = JVM_SUCCESS;
            break;
          }
        }
        if (error > 0) {
          debugf("##>field not found\n");
          break;
        }
        x += 3;
        break;
      /// putstatic
      case 0xb3:
        // name
        y = code[x+1] << 8 | code[x+2];
        // value
        jvm_StackPop(&stack, &result);

        f = (JVMConstPoolFieldRef*)jclass->pool[y - 1];
        d = (JVMConstPoolNameAndType*)jclass->pool[f->nameAndTypeIndex - 1];
        a = (JVMConstPoolUtf8*)jclass->pool[d->nameIndex - 1];
        //tmp = a->string;

        // look through obj's fields until we find
        // a matching entry then check the types
        for (w = 0; w < jclass->sfieldCnt; ++w) {
          if (jvm_strcmp(jclass->sfields[w].name, a->string) == 0) {
            // matched name now check type
            if (jclass->sfields[w].flags & JVM_STACK_ISOBJECTREF)
            {
              // see if it is instance of class type specified
              if (jvm_IsInstanceOf(bundle, ((JVMObject*)result.data), jvm_GetClassNameFromClass(jclass->sfields[w].jclass))) {
                  debugf("not instance of..\n");
                  error = JVM_ERROR_BADCAST;
                  break;
              }
            } else {
              //if (jclass->sfields[w].flags != result.flags) {
              //  debugf("** %x / %x\n", jclass->sfields[w].flags, result.flags);
              //  jvm_exit(4);
              //  error = JVM_ERROR_FIELDTYPEDIFFERS;
              //  break;
              // }
            }

            if (error < 0)
              break;

            // for the previous object we are about to overwrite
            // we need to do the same except decrement and unlink
            // if refcnt is 0

            // actualy store it now
            jclass->sfields[w].value = (intptr)result.data;
            jclass->sfields[w].aflags = result.flags;
            debugf("putstatic jclass->sfields[w].value:%li\n", jclass->sfields[w].value);

            break;
          }

        }

        // if error go handle it
        if (error < 0)
          break;

        x += 3;
        break;
      /// putfield
      case 0xb5:
        // name
        y = code[x+1] << 8 | code[x+2];
        // value
        jvm_StackPop(&stack, &result);
        // object (the object whos field we are setting)
        jvm_StackPop(&stack, &result2);
        _jobject = (JVMObject*)result2.data;
        _jclass = _jobject->class;
        debugf("here %u\n", _jclass->pool[y - 1]->type);
        if (_jclass->pool[y - 1]->type == 1)
        {
          debugf("-->%s\n", ((JVMConstPoolUtf8*)_jclass->pool[y - 1])->string);
          a = (JVMConstPoolUtf8*)_jclass->pool[y - 1];
        } else {
          f = (JVMConstPoolFieldRef*)_jclass->pool[y - 1];
          debugf("here %u\n", f->nameAndTypeIndex);
          d = (JVMConstPoolNameAndType*)_jclass->pool[f->nameAndTypeIndex - 1];
          debugf("here\n");
          a = (JVMConstPoolUtf8*)_jclass->pool[d->nameIndex - 1];
          debugf("##>%s\n", a->string);
          //jvm_exit(-4);
        }

        error = jvm_PutField(bundle, (JVMObject*)result2.data, a->string, result.data, result.flags);
        debugf("done\n");
        if (error != JVM_SUCCESS)
          break;
        
        x += 3;
        break;
      /// tableswitch
      case 0xaa:
        jvm_StackPop(&stack, &result);
        w = (uintptr)&code[x];
        if (w & 0x3)
          w = (w & ~0x3) + 4;
        else
          w += 4;
        debugf("code[%x]%lx w:%lx\n", x, (uintptr)&code[x], w);

        map = (uint32*)(w);

        debugf("db:%i lb:%i hb:%i\n", nothl(map[0]), nothl(map[1]), nothl(map[2]));
        
        // too low
        if ((int32)result.data < (int32)nothl(map[1])) {
          debugf("too low\n");
          x += (int32)nothl(map[0]);
          break;
        }
        // too high
        if ((int32)result.data > (int32)nothl(map[2])) {
          debugf("too high\n");
          x += (int32)nothl(map[0]);
          break;
        }
        w = ((int32)result.data - (int32)nothl(map[1]));
        x += nothl(map[3 + w]);
        debugf("ok:%i\n", nothl(map[3 + w]));
        break;
      /// lookupswitch
      case 0xab:
        jvm_StackPop(&stack, &result);
        w = (uintptr)&code[x];
        if (w & 0x3)
          w = (w & ~0x3) + 4;
        else
          w += 4;
        debugf("code[%x]%lx w:%lx\n", x, (uintptr)&code[x], w);
        
        map = (uint32*)(w);

        debugf("x:%u w:%u\n", (uintptr)&code[x], w);
        debugf("--- %x %i %i\n", nothl(map[0]), nothl(map[1]), nothl(map[2]));
        for (y = 0; y < nothl(map[1]); ++y) {
           debugf("map:%i:%i\n", nothl(map[y*2+2]), nothl(map[y*2+3]));
        }
        for (y = 0; y < nothl(map[1]); ++y) {
          v = (int32)nothl(map[y*2+3]);
          k = nothl(map[y*2+2]);
          debugf("_map:%i:%i:%i\n", k, v, result.data);
          if ((uint32)result.data == k) {
            debugf("match %i %i\n", result.data, k, v);
            x += (int32)v;
            break;
          }
        }
        if (y >= nothl(map[1]))
          x += (int32)nothl(map[0]);
        //debugf("end switch\n");
        //jvm_exit(-6);
        break;
      /// invokestatic
      case 0xb8:
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
         //if (jvm_strcmp(mclass, "java/lang/Object") == 0) {
         // debugf("caught java/lang/Object call and skipped it\n");
         // x +=3 ;
         // break;
         //}

         d = (JVMConstPoolNameAndType*)jclass->pool[b->descIndex - 1];
         a = (JVMConstPoolUtf8*)jclass->pool[d->nameIndex - 1];
         // a->string is the method of the class
         _jclass = jvm_FindClassInBundle(bundle, mclass);
         debugf("here %x\n", _jclass);
         if (!_jclass) {
           debugf("for method call can not find class [%s]\n", mclass);
           error = JVM_ERROR_CLASSNOTFOUND;
           break;
         }

         debugf("find class in bundle %s:%s\n", mclass, jvm_GetClassNameFromClass(_jclass));

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
           if (!_jclass)
           {
             error = JVM_ERROR_SUPERMISSING;
             debugf("Could not find super class in bundle!?!\n");
             break;
           }
         }

         argcnt = jvm_GetMethodTypeArgumentCount(mtype);

         debugf("##>invoke: %s:%s[%u] in %s\n", mmethod, mtype, argcnt, mclass);

         jvm_DebugStack(&stack);

         /// pop locals from stack into local variable array
         _locals = (JVMLocal*)jvm_malloc(sizeof(JVMLocal) * (argcnt + 2));

         w = 1;
         if (opcode == 0xb8)
           w = 0;
        
        for (y = 0; y < argcnt; ++y) {
           jvm_StackPop(&stack, &result);
           debugf("##>farg %x %x\n", result.data, result.flags);
           _locals[(argcnt - y - 1) + w].data = result.data;
           _locals[(argcnt - y - 1) + w].flags = result.flags;
         }

         // if not static invocation then we need the objref
         if (opcode != 0xb8) {
          /// pop object reference from stack
          jvm_StackPop(&stack, &result);
          if (!(result.flags & JVM_STACK_ISOBJECTREF)) {
            error = JVM_ERROR_NOTOBJREF;
            debugf("object from stack is not object reference!");
            break;
          }
          debugf("CLAZZ:%s\n", jvm_GetClassNameFromClass(((JVMObject*)result.data)->class));
         }
         // if not static invocation then we need the objref
         if (opcode != 0xb8) {
          _locals[0].data = result.data;
          _locals[0].flags = result.flags;
          // only parameters counted, so if this is
          // not a static call we need to include
          // our object(self) also
          argcnt += 1;
          debugf("objref->stackCnt:%u\n", ((JVMObject*)_locals[0].data)->stackCnt);
          debugf("calling method with self.data:%x self.flags:%u\n", _locals[0].data, _locals[0].flags);
         }
         
         // check if this is a special native implemented object
         debugf("accessFlags:%x\n", _method->accessFlags);
         if ((_jclass->flags & JVM_CLASS_NATIVE) && (_method->accessFlags & JVM_ACC_NATIVE)) {
           debugf("-----native-call----\n");
           // get native implementation procedure index
           eresult = _jclass->nhand(jvm, bundle, _jclass, mmethod, mtype, _locals, argcnt, &result);
           //eresult = jvm->nprocs[w](jvm, bundle, _jclass, mmethod, mtype, _locals, argcnt + 1, &result); 
         } else {
           debugf("-----java-call----\n");
           eresult = jvm_ExecuteObjectMethod(jvm, bundle, _jclass, mmethod, mtype, _locals, argcnt, &result);
         }
         jvm_free(_locals);

         debugf("##out\n");

         // the called method had an exception thrown and it was unable
         // to handle it so now we need to setup to see if we can handle
         // it, this will cause us to enter into the exception control
         // code block a ways below
         if (eresult < 0) {
           jvm_StackPush(&stack, result.data, result.flags);
           error = JVM_ERROR_EXCEPTION;
           debugf("propagating error down the stack frames..\n");
           break;
         }

         // if the function return type if void then we
         // do not place anything onto the stack
         if (!jvm_IsMethodReturnTypeVoid(mtype)) {
          /// push result onto stack
          debugf("return type not void!\n");
          jvm_StackPush(&stack, result.data, result.flags);
         } else {
           debugf("return type void..\n");
         }
         // continue executing..
         x += 3;
         break;
      /// areturn: return reference from a method
      case 0xb0:
      /// ireturn: return integer from method
      case 0xac:
      debugf("return int from method\n");
      /// return: void from method
      case 0xb1:
         if (opcode != 0xb1)
         {
          // _result is our return value structure
          jvm_DebugStack(&stack);
          jvm_StackPop(&stack, _result);
         }

         // we stored the object reference of the class object
         // for this method in locals[0] which is where then
         // java compiler expects it to be now we need to scrub
         // it's fields to sync object stack counts
         debugf("scrubbing fields, stack, and locals..\n");
         /// should i be scrubbing fields??? i dont think so..? not sure
         //if (locals[0].flags & JVM_STACK_ISOBJECTREF)
         // jvm_ScrubObjectFields(locals[0].data);
         jvm_ScrubStack(&stack);
         debugf("##out1\n");
         jvm_ScrubLocals(locals, method->code->maxLocals);
         debugf("##out2 data:%x flags:%x\n", stack.data, stack.flags);
         jvm_StackFree(&stack);
         jvm_free(locals);
         return JVM_SUCCESS;
      default:
        debugf("unknown opcode %x\n", opcode);
        jvm_exit(-3);
        return JVM_ERROR_UNKNOWNOPCODE;
    }
    /// ---------------------------------
    /// END OF SWITCH STATEMENT
    /// ---------------------------------
    jvm_DebugStack(&stack);
    debugf("error:%i\n", error);
    // either a exception object was thrown (JVM_ERROR_EXCEPTION) from
    // the athrow opcode or a run-time exception occured and the type
    // of it is stored in error
    if (error < 0) {
      debugf("got exception -- scrubing locals and stack\n");
      /// these are run-time exceptions
      if (error != JVM_ERROR_EXCEPTION) {
        _error = jvm_CreateObject(jvm, bundle, "java/lang/Exception", &_jobject);
        // this block lets me create a specific exception from the error code
        // and it keeps me from having to implement complex blocks in each
        // opcode that needs to throw a runtime exception, once we create
        // the exception we treat it like a regular thrown exception and it
        // can propagate down the stack
        if (_error < 0) {
          debugf("Could not find java/lang/Exception!\n");
          jvm_exit(_error);
        }
        _jobject->stackCnt = 0;
      } else {
        // this is where a regular exception is caught, normally
        // from the athrow opcode, but if one is passed down
        // it will also land here also
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
      }
      // make sure the stack is scrubed clean and the locals?
      jvm_ScrubStack(&stack);
      jvm_ScrubLocals(locals, method->code->maxLocals);
      // now we need to check if there is an exception handler
      // which can take control or if not we will pass it down
      debugf("checking if inside exception handler..\n");
      for (y = 0; y < method->code->eTableCount; ++y) {
        debugf("  check x:%i pcStart:%i pcEnd:%i\n", x, method->code->eTable[y].pcStart, method->code->eTable[y].pcEnd);
        if (x >= method->code->eTable[y].pcStart)
          if (x < method->code->eTable[y].pcEnd) {
            c = (JVMConstPoolClassInfo*)jclass->pool[method->code->eTable[y].catchType - 1];
            a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
            debugf("catchType:%s\n", a->string);
            /// is _jobject an instance of exception handler class a->string?
            if (!jvm_IsInstanceOf(bundle, _jobject, a->string)) {
              /// yes, then jump to exception handler
              jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISOBJECTREF);
              x = method->code->eTable[y].pcHandler;
              debugf("jumping to pcHandler:%i\n", x);
              error = JVM_SUCCESS;
              break;
            }
          }
      }
      // if error is still set then we need to pass it down because the
      // block above was unable to find a handler which matched
      if (error < 0) {
        // free the stack and locals array that we created when
        // we entered into this procedure, then set exceptions
        // as a return value and exit out the code waiting
        // will enter right back into this exception handling
        // block until the program terminates or it is caught
        jvm_StackFree(&stack);
        jvm_free(locals);
        _result->data = (uint64)_jobject;
        _result->flags = JVM_STACK_ISOBJECTREF;
        debugf("Passing exception down the stack..\n");
        return JVM_ERROR_EXCEPTION;
      }
    }
    /// END OF ERROR MANAGEMENT
   }
  /// END OF LOOP
  // we never should make it here
  jvm_printf("[error] reached end of loop!?\n");
  jvm_exit(-55);
  return JVM_SUCCESS;
}
