#include "rjvm.h"
#include "exec.h"

void jvm_LocalPut(JVMLocal *locals, uint32 ndx, uintptr data, uint32 flags) {
  if (locals[ndx].flags & JVM_STACK_ISOBJECTREF)
    if (locals[ndx].data)
      ((JVMObject*)locals[ndx].data)->stackCnt--;
  locals[ndx].data = data;
  locals[ndx].flags = flags;
  if (flags & JVM_STACK_ISOBJECTREF)
    if (data)
      ((JVMObject*)data)->stackCnt++;
}

/// create primitive array
int jvm_CreatePrimArray(JVM *jvm, JVMBundle *bundle, uint8 type, uint32 cnt, JVMObject **jobject) {
  JVMObject             *_jobject;

  // JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF
  _jobject = (JVMObject*)malloc(sizeof(JVMObject));
  _jobject->next = jvm->objects;
  jvm->objects = _jobject;
  _jobject->class = jvm_FindClassInBundle(bundle, "java/lang/Array");
  if (!_jobject->class) {
    debugf("whoa.. newarray but java/lang/Array not in bundle!\n");
    exit(-99);
  }
  _jobject->fields = 0;

  _jobject->stackCnt = 0;

  switch(type) {
    case JVM_ATYPE_LONG:
      _jobject->fields = (uint64*)malloc(sizeof(uint64) * cnt);
      break;
    case JVM_ATYPE_INT:
      _jobject->fields = (uint64*)malloc(sizeof(uint32) * cnt);
      break;
    case JVM_ATYPE_CHAR:
      _jobject->fields = (uint64*)malloc(sizeof(uint8) * cnt);
      break;
    case JVM_ATYPE_BYTE:
      _jobject->fields = (uint64*)malloc(sizeof(uint8) * cnt);
      break;
    case JVM_ATYPE_FLOAT:
      _jobject->fields = (uint64*)malloc(sizeof(uint32) * cnt);
      break;
    case JVM_ATYPE_DOUBLE:
      _jobject->fields = (uint64*)malloc(sizeof(uint64) * cnt);
      break;
    case JVM_ATYPE_BOOL:
      _jobject->fields = (uint64*)malloc(sizeof(uint8) * cnt);
      break;
    case JVM_ATYPE_SHORT:
      _jobject->fields = (uint64*)malloc(sizeof(uint16) * cnt);
      break;
  }
  _jobject->fieldCnt = (uintptr)cnt;
  //jvm_StackPush(&stack, (uint64)_jobject, (y << 4) | JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
  debugf("just created array\n");
  //jvm_DebugStack(&stack);
  *jobject = _jobject;

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
  int32                         _error;
  JVMLocal                      result;
  JVMLocal                      result2;
  int32                         y, w, z, g;
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
  //  printf("g_dbg_ec=%u\n", g_dbg_ec);
  //  exit(-9);
  //}

  jvm_StackInit(&stack, 1024);
  error = 0;

  debugf("executing %s\n", methodName);

  /// find method specifiee
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

  debugf("method has code(%lx) of length %u\n", code, codesz);
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
    debugf("$$data:%u flags:%u\n", _locals[x].data, _locals[x].flags);
    if (locals[x].flags & JVM_STACK_ISOBJECTREF)
      if (locals[x].data)
        ((JVMObject*)locals[x].data)->stackCnt++;
  }
  /// zero the remainder of the locals
  for (; x < 256; ++x) {
    locals[x].data = 0;
    locals[x].flags = 0;
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
            debugf("***********************************\n");
            jvm_CreateObject(jvm, bundle, "java/lang/String", &_jobject);
            // create byte array to hold string
            /// todo: ref our byte[] to String
            /// todo: also do for putfield and getfield opcodes
            jvm_CreatePrimArray(jvm, bundle, JVM_ATYPE_BYTE, w, &__jobject);
            __jobject->stackCnt = 1;
            debugf("_jobject->_fields:%x\n", _jobject->_fields);
            for (w = 0; w < _jobject->fieldCnt; ++w) {
              debugf("%s.%s.%u\n", _jobject->_fields[w].name, "string", w);
              if (strcmp(_jobject->_fields[w].name, "string") == 0) {
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
            //exit(-3);
            break;
          case TAG_INTEGER:
            jvm_StackPush(&stack, (intptr)((JVMConstPoolInteger*)jclass->pool[y - 1])->value, JVM_STACK_ISINT);
            break;
          case TAG_FLOAT:
            debugf("TAG_FLOAT not implemented!\n");
            exit(-9);
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
        if (!jvm_IsInstanceOf(bundle, _jobject, mclass))
        {
          debugf("bad cast to %s\n", mclass);
          debugf("i am here\n");
          exit(-9);
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
        /// classinfo
        c = (JVMConstPoolClassInfo*)jclass->pool[y - 1];
        a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
        error = jvm_CreateObject(jvm, bundle, a->string, &_jobject);
        /// jump out and create exception if we can..
        if (error < 0)
          break;
        _jobject->stackCnt = 0;
        jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISOBJECTREF);
        x += 3;
        break;
      /// goto
      case 0xa7:
        y = code[x+1] << 8 | code[x+2];
        x = x + (int16)y;
        break;
      // i2b
      case 0x91:
        jvm_StackPop(&stack, &result);
        result.data = (uint8)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      // i2c
      case 0x92:
        jvm_StackPop(&stack, &result);
        result.data = (uint8)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      // i2d
      case 0x87:
        jvm_StackPop(&stack, &result);
        ((double*)result.data)[0] = (double)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      // i2f
      case 0x86:
        jvm_StackPop(&stack, &result);
        ((float*)result.data)[0] = (float)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      // i2l
      case 0x85:
        jvm_StackPop(&stack, &result);
        result.data = (int64)(int32)result.data;
        jvm_StackPush(&stack, result.data, result.flags);
        x += 1;
        break;
      // i2s
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
        jvm_LocalPut(locals, 2, result.data, result.flags);
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
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y | w, JVM_STACK_ISINT);
        x += 1;
        break;      
      /// iushr: logical shift right on int
      case 0x7c:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, ~(~0 >> w) | (y >> w), JVM_STACK_ISINT);
        x += 1;
        break;        
      /// irem: logical int remainder
      case 0x70:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y % w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// ishl
      case 0x78:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y << w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// ishr
      case 0x7a:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y >> w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// isub
      case 0x64:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y - w, JVM_STACK_ISINT);
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
        jvm_StackPush(&stack, y / w, JVM_STACK_ISINT);
        x += 1;
        break;
      /// imul: multiply two ints
      case 0x68:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y * w, JVM_STACK_ISINT);
        x += 1;
        break;        
      /// iadd: add two ints
      case 0x60:
        jvm_StackPop(&stack, &result);
        y = result.data;
        jvm_StackPop(&stack, &result);
        w = result.data;
        jvm_StackPush(&stack, y + w, JVM_STACK_ISINT);
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
          if (!jvm_IsInstanceOf(bundle, __jobject, jvm_GetClassNameFromClass(_jobject->class))) {
            error = JVM_ERROR_WASNOTINSTANCEOF;
            break;
          }
        }
       
        if (w >= (((uint64*)_jobject->fields)[0])) {
          // error, past end of array..
          error = JVM_ERROR_ARRAYOUTOFBOUNDS;
          break;
        }
        // if we are overwritting something and it is going
        // to be null or an object so decrement its refcnt
        if (((JVMObject**)_jobject->fields)[w + 1] != 0)
          ((JVMObject**)_jobject->fields)[w + 1]->stackCnt--;

        ((JVMObject**)_jobject->fields)[w + 1] = __jobject;
        __jobject->stackCnt++;
        
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
        if (w >= (((uint64*)_jobject->fields)[0])) {
          /// error, past end of array..
          error = JVM_ERROR_ARRAYOUTOFBOUNDS;
          break;
        }

        __jobject = ((JVMObject**)_jobject->fields)[w + 1];
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
        _jobject = (JVMObject*)malloc(sizeof(JVMObject));
        _jobject->next = jvm->objects;
        jvm->objects = _jobject;
        /// allows us to know if this is a ref array for a type
        /// or if it is a primitive array=0 while obj array!=0
        c = (JVMConstPoolClassInfo*)jclass->pool[y - 1];
        a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
        _jobject->class = jvm_FindClassInBundle(bundle, a->string);
        _jobject->refs = 0;
        _jobject->stackCnt = 0;
        jvm_StackPop(&stack, &result);
        argcnt = result.data;
        _jobject->fields = (uint64*)malloc(sizeof(JVMObject*) * (argcnt + 1));
        ((uintptr*)_jobject->fields)[0] = argcnt;
        jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
        x += 3;
        break;
      /// newarray
      case 0xbc:
        /// hack around JVMObject so reference counting
        /// logic can still work, might need some unions
        /// to clean up dirty code below --kmcguire
        y = code[x+1];
        jvm_StackPop(&stack, &result);
        jvm_CreatePrimArray(jvm, bundle, y, result.data, &_jobject);
        jvm_StackPush(&stack, (uintptr)_jobject, (y << 4) | JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
        x += 2;
        break;
      /// sipush: push a short onto the stack
      case 0x11:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPush(&stack, (intptr)y, JVM_STACK_ISSHORT);
        x += 3;
        break;
      /// bipush: push a byte onto the stack as an integer
      case 0x10:
        y = (int8)code[x+1];
        jvm_StackPush(&stack, (intptr)y, JVM_STACK_ISINT);
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
        
        f = (JVMConstPoolFieldRef*)_jclass->pool[y - 1];
        d = (JVMConstPoolNameAndType*)_jclass->pool[f->nameAndTypeIndex - 1];
        a = (JVMConstPoolUtf8*)_jclass->pool[d->nameIndex - 1];

        //
        
        error = 1;
        for (w = 0; w < _jobject->fieldCnt; ++w) {
          debugf("looking for field %s have %s\n", a->string, _jobject->_fields[w].name);
          if (strcmp(_jobject->_fields[w].name, a->string) == 0) {
            // push onto the stack
            jvm_StackPush(&stack, _jobject->_fields[w].value, _jobject->_fields[w].aflags);
            error = 0;
            break;
          }
        }
        if (error > 0) {
          debugf("field not found\n");
          exit(-7);
          break;
        }
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
        f = (JVMConstPoolFieldRef*)_jclass->pool[y - 1];
        d = (JVMConstPoolNameAndType*)_jclass->pool[f->nameAndTypeIndex - 1];
        a = (JVMConstPoolUtf8*)_jclass->pool[d->nameIndex - 1];
        //tmp = a->string;
        
        // look through obj's fields until we find
        // a matching entry then check the types
        for (w = 0; w < _jobject->fieldCnt; ++w) {
          if (strcmp(_jobject->_fields[w].name, a->string) == 0) {
            // matched name now check type
            if (_jobject->_fields[w].flags & JVM_STACK_ISOBJECTREF)
            {
              // see if it is instance of class type specified
              if (!jvm_IsInstanceOf(bundle, ((JVMObject*)result.data), jvm_GetClassNameFromClass(_jobject->_fields[w].jclass))) {
                  debugf("not instance of..\n");
                  error = JVM_ERROR_BADCAST;
                  break;
              }
            } else {
              if (_jobject->_fields[w].flags != result.flags) {
                debugf("** %u / %u\n", _jobject->_fields[w].flags, result.flags);
                error = JVM_ERROR_FIELDTYPEDIFFERS;
                break;
              }
            }

            if (error)
              break;
            
            // decrement existing object if there
            if (_jobject->_fields[w].flags & JVM_STACK_ISOBJECTREF)
              if (_jobject->_fields[w].value)
                ((JVMObject*)_jobject->_fields[w].value)->stackCnt--;
            // increment object we are placing there
            if (result.flags & JVM_STACK_ISOBJECTREF)
              if (result.data)
                ((JVMObject*)result.data)->stackCnt++;
            // go through and find our link in the objet's ref links
              // increment refcnt
            // if no link create one and set refcnt to 1

            // for the previous object we are about to overwrite
            // we need to do the same except decrement and unlink
            // if refcnt is 0
              
            // actualy store it now
            _jobject->_fields[w].value = (uintptr)result.data;
            _jobject->_fields[w].aflags = result.flags;
            
            break;
          }
          
        }
        
        // if error go handle it
        if (error)
          break;

        x += 3;
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
         //if (strcmp(mclass, "java/lang/Object") == 0) {
         // debugf("caught java/lang/Object call and skipped it\n");
         // x +=3 ;
         // break;
         //}

         d = (JVMConstPoolNameAndType*)jclass->pool[b->descIndex - 1];
         a = (JVMConstPoolUtf8*)jclass->pool[d->nameIndex - 1];
         // a->string is the method of the class
         _jclass = jvm_FindClassInBundle(bundle, mclass);
         debugf("find class in bundle %s:%s\n", mclass, jvm_GetClassNameFromClass(_jclass));
         
         if (!_jclass) {
           debugf("for method call can not find class [%s]\n", mclass);
           error = JVM_ERROR_CLASSNOTFOUND;
           break;
         }

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

         debugf("invoke: %s:%s[%u] in %s\n", mmethod, mtype, argcnt, mclass);

         jvm_DebugStack(&stack);

         /// pop locals from stack into local variable array
         _locals = (JVMLocal*)malloc(sizeof(JVMLocal) * (argcnt + 2));
         w = 1;
         if (opcode == 0xb8)
           w = 0;
         for (y = 0; y < argcnt; ++y) {
           jvm_StackPop(&stack, &result);
           _locals[y + w].data = result.data;
           _locals[y + w].flags = result.flags;
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

          debugf("objref->stackCnt:%u\n", ((JVMObject*)_locals[0].data)->stackCnt);
          debugf("calling method with self.data:%x self.flags:%u\n", _locals[0].data, _locals[0].flags);
         }
         debugf("here\n");
         // check if this is a special native implemented object
         debugf("accessFlags:%x\n", _method->accessFlags);
         if ((_jclass->flags & JVM_CLASS_NATIVE) && (_method->accessFlags & JVM_ACC_NATIVE)) {
           debugf("-----native-call----\n");
           // get native implementation procedure index
           eresult = _jclass->nhand(jvm, bundle, _jclass, mmethod, mtype, _locals, argcnt + 1, &result);
           //eresult = jvm->nprocs[w](jvm, bundle, _jclass, mmethod, mtype, _locals, argcnt + 1, &result); 
         } else {
           debugf("-----java-call----\n");
           eresult = jvm_ExecuteObjectMethod(jvm, bundle, _jclass, mmethod, mtype, _locals, argcnt + 1, &result);
         }
         free(_locals);

         if (eresult < 0) {
           error = eresult;
           debugf("propagating error down the stack frames..\n");
           break;
         }

         /// need to know if it was a void return or other
         if (!jvm_IsMethodReturnTypeVoid(mtype)) {
          /// push result onto stack
          debugf("return type not void!\n");
          jvm_StackPush(&stack, result.data, result.flags);
         } else {
           debugf("return type void..\n");
         }

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
         /// should i be scrubbing fields??? i dont think so..
         //if (locals[0].flags & JVM_STACK_ISOBJECTREF)
         // jvm_ScrubObjectFields(locals[0].data);
         jvm_ScrubStack(&stack);
         jvm_ScrubLocals(locals);
         jvm_StackFree(&stack);
         free(locals);
         return JVM_SUCCESS;
      default:
        debugf("unknown opcode %x\n", opcode);
        exit(-3);
        return JVM_ERROR_UNKNOWNOPCODE;
    }
    /// ---------------------------------
    /// END OF SWITCH STATEMENT
    /// ---------------------------------
    jvm_DebugStack(&stack);
    debugf("error:%i\n", error);
    /// we encountered an error condition which could be
    /// handled by an exception so we setup for it here
    if (error < 0) {
      debugf("got exception -- scrubing locals and stack\n");
      /// these are run-time exceptions
      if (error != JVM_ERROR_EXCEPTION) {
        _error = jvm_CreateObject(jvm, bundle, "java/lang/Exception", &_jobject);
        _jobject->stackCnt = 0;
        if (_error < 0) {
          debugf("could not create object type %s!\n");
          exit(_error);
        }
      } else {
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
      }

      jvm_ScrubStack(&stack);
      jvm_ScrubLocals(locals);
      /// are we between an exception handler?
      debugf("checking if inside exception handler..\n");
      for (y = 0; y < method->code->eTableCount; ++y) {
        debugf("  check x:%i pcStart:%i pcEnd:%i\n", x, method->code->eTable[y].pcStart, method->code->eTable[y].pcEnd);
        if (x >= method->code->eTable[y].pcStart)
          if (x < method->code->eTable[y].pcEnd) {
            c = (JVMConstPoolClassInfo*)jclass->pool[method->code->eTable[y].catchType - 1];
            a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
            debugf("catchType:%s\n", a->string);
            /// is _jobject an instance of exception handler class a->string?
            if (jvm_IsInstanceOf(bundle, _jobject, a->string)) {
              /// yes, then jump to exception handler
              jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISOBJECTREF);
              x = method->code->eTable[y].pcHandler;
              debugf("jumping to pcHandler:%i\n", x);
              error = 0;
              break;
            }
          }
      }
      /// if we jumped to an exception handler then we
      /// need to try to continue
      if (error < 0) {
        jvm_StackFree(&stack);
        free(locals);
        debugf("A run-time exception occured as type %i\n", error);
        return error;
      }
    }
    /// END OF ERROR MANAGEMENT
   }
  /// END OF LOOP
  // We should technically never make it here. Since
  // a return opcode should bring us out. So let us
  // make it an error to arrive here.
  printf("[error] reached end of loop!?\n");
  exit(-55);
  return JVM_SUCCESS;
}
