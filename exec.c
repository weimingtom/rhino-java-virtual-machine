#include "rjvm.h"
#include "exec.h"

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
    debugf("opcode(%u/%u):%x\n", x, codesz, opcode);
    switch (opcode) {
      /// nop: no operation
      case 0:
        x += 2;
        break;
      /*
      /// ldc: push a constant #index from a constant pool (string, int, or float) onto the stack
      case 0x12:
        y = code[x+1];
        /// determine what this index refers too
        switch (jclass->pool[y]->type) {
          case TAG_STRING:
            /// create string object
            jvm_CreateObject(jvm, bundle, "java/lang/String", &_jobject);
            /// create byte array
            _jobject->
            /// set byte array to String field
            
          case TAG_INTEGER:
            jclass->pool
            jvm_StackPush(&stack,
          case TAG_FLOAT:
        }
        x += 2;
        break;
      /// ldc_w:
      /// ldc2_w:
      */
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
        
        /// is objref the type described by Y (or can be)
        if (!jvm_IsInstanceOf(bundle, _jobject, mclass))
        {
          debugf("bad cast to %s\n", mclass);
          error = JVM_ERROR_BADCAST;
          break;
        }
        debugf("good cast to %s\n", mclass);
        x += 3;
        break;
      /// if_icmpgt
      case 0xa4:
        y = (int16)(code[x+1] << 8 | code[x+2]);
        jvm_StackPop(&stack, &result2);
        jvm_StackPop(&stack, &result);
        debugf("compare %i <= %i\n", result.data, result2.data);
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
        if (result.flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)result.data)->stackCnt++;
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
        _jobject->stackCnt = 1;
        jvm_StackPush(&stack, (uint64)_jobject, JVM_STACK_ISOBJECTREF);
        x += 3;
        break;
      /// goto
      case 0xa7:
        y = code[x+1] << 8 | code[x+2];
        debugf("goto:%u\n", y);
        x = x + y;
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
      /// astore
      case 0x3a:
        y = code[x+1];
        jvm_StackPop(&stack, &result);
        if (locals[y].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[y].data)->stackCnt--;
        locals[y].data = result.data;
        locals[y].flags = result.flags;
        x += 2;
        break;
      /// astore_0
      case 0x4b:
        jvm_StackPop(&stack, &result);
        if (locals[0].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[y].data)->stackCnt--;
        locals[0].data = result.data;
        locals[0].flags = result.flags;
        x += 1;
        break;
      /// astore_1
      case 0x4c:
        jvm_StackPop(&stack, &result);
        if (locals[1].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[y].data)->stackCnt--;
        locals[1].data = result.data;
        locals[1].flags = result.flags;
        x += 1;
        break;
      /// astore_2
      case 0x4d:
        jvm_StackPop(&stack, &result);
        if (locals[2].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[y].data)->stackCnt--;
        locals[2].data = result.data;
        locals[2].flags = result.flags;
        x += 1;
        break;
      /// astore_3
      case 0x4e:
        jvm_StackPop(&stack, &result);
        if (locals[3].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[y].data)->stackCnt--;
        locals[3].data = result.data;
        locals[3].flags = result.flags;
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
        
        if (w >= (uint64)_jobject->class) {
          /// error, past end of array..
          error = JVM_ERROR_ARRAYOUTOFBOUNDS;
          break;
        }
        debugf("here\n");
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
          /// baload: load byte/boolean from arraylength
          case 0x33:
            jvm_StackPush(&stack, ((uint8*)_jobject->fields)[w], JVM_STACK_ISOBJECTREF);
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
        if (w >= (uint64)_jobject->class) {
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
        jvm_StackPop(&stack, &result);
        _jobject = (JVMObject*)result.data;
        debugf("_jobject:%x\n", _jobject);
        if (!_jobject) {
          error = JVM_ERROR_NULLOBJREF;
          break;
        }
        jvm_StackPush(&stack, (uint64)_jobject->class, JVM_STACK_ISINT);
        x += 1;
        break;
      /// aastore: store ref into ref array
      case 0x53:
        debugf("OK\n");
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
        if (__jobject)
          __jobject->stackCnt++;
        if (((JVMObject**)_jobject->fields)[w + 1] != 0)
          ((JVMObject**)_jobject->fields)[w + 1]->stackCnt--;

        ((JVMObject**)_jobject->fields)[w + 1] = __jobject;
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
          __jobject->stackCnt++;
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
        debugf("setting objref array type to %s\n", a->string);
        _jobject->class = jvm_FindClassInBundle(bundle, a->string);
        _jobject->refs = 0;
        _jobject->stackCnt = 1;
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
        // JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF
        _jobject = (JVMObject*)malloc(sizeof(JVMObject));
        _jobject->next = jvm->objects;
        jvm->objects = _jobject;
        _jobject->class = 0;
        _jobject->fields = 0;
        _jobject->refs = 0;
        _jobject->stackCnt = 1;

        jvm_StackPop(&stack, &result);
        argcnt = result.data;

        switch(y) {
          case JVM_ATYPE_LONG:
            _jobject->fields = (uint64*)malloc(sizeof(uint64) * argcnt);
            y = JVM_STACK_ISLONG;
            break;
          case JVM_ATYPE_INT:
            _jobject->fields = (uint64*)malloc(sizeof(uint32) * argcnt);
            y = JVM_STACK_ISINT;
            break;
          case JVM_ATYPE_CHAR:
            _jobject->fields = (uint64*)malloc(sizeof(uint8) * argcnt);
            y = JVM_STACK_ISCHAR;
            break;
          case JVM_ATYPE_BYTE:
            _jobject->fields = (uint64*)malloc(sizeof(uint8) * argcnt);
            y = JVM_STACK_ISBYTE;
            break;
          case JVM_ATYPE_FLOAT:
            _jobject->fields = (uint64*)malloc(sizeof(uint32) * argcnt);
            y = JVM_STACK_ISFLOAT;
            break;
          case JVM_ATYPE_DOUBLE:
            _jobject->fields = (uint64*)malloc(sizeof(uint64) * argcnt);
            y = JVM_STACK_ISDOUBLE;
            break;
          case JVM_ATYPE_BOOL:
            _jobject->fields = (uint64*)malloc(sizeof(uint8) * argcnt);
            y = JVM_STACK_ISBOOL;
            break;
          case JVM_ATYPE_SHORT:
            _jobject->fields = (uint64*)malloc(sizeof(uint16) * argcnt);
            y = JVM_STACK_ISSHORT;
            break;
        }
        _jobject->class = (JVMClass*)(uintptr)argcnt;
        jvm_StackPush(&stack, (uint64)_jobject, y | JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF);
        debugf("just created array\n");
        jvm_DebugStack(&stack);
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
        if (result.flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)result.data)->stackCnt--;
        x += 1;
        break;
      /// aload: load a reference onto the stack from local variable 'y'
      case 0x19:
        y = code[x+1];
        if (locals[y].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[y].data)->stackCnt--;
        jvm_StackPush(&stack, locals[y].data, locals[y].flags);
        if (locals[x].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[0].data)->stackCnt++;
        x += 2;
        break;
      /// aload_0: load a reference onto the stack from local variable 0
      case 0x2a:
        if (locals[0].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[0].data)->stackCnt--;
        jvm_StackPush(&stack, locals[0].data, locals[0].flags);
        if (locals[0].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[0].data)->stackCnt++;
        x += 1;
        break;
      /// aload_1
      case 0x2b:
        if (locals[1].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[1].data)->stackCnt--;
        jvm_StackPush(&stack, locals[1].data, locals[1].flags);
        if (locals[1].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[1].data)->stackCnt++;
        x += 1;
        break;
      /// aload_2
      case 0x2c:
        if (locals[2].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[2].data)->stackCnt--;
        jvm_StackPush(&stack, locals[2].data, locals[2].flags);
        if (locals[2].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[2].data)->stackCnt++;
        x += 1;
        break;
      /// aload_3
      case 0x2d:
        if (locals[3].flags & JVM_STACK_ISOBJECTREF)
          ((JVMObject*)locals[3].data)->stackCnt--;
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
         //if (strcmp(mclass, "java/lang/Object") == 0) {
         // debugf("caught java/lang/Object call and skipped it\n");
         // x +=3 ;
         // break;
         //}

         d = (JVMConstPoolNameAndType*)jclass->pool[b->descIndex - 1];
         a = (JVMConstPoolUtf8*)jclass->pool[d->nameIndex - 1];
         // a->string is the method of the class
         debugf("looking!!!\n");
         _jclass = jvm_FindClassInBundle(bundle, mclass);
         debugf("_jclass:%x\n", _jclass);
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
         debugf("class with implementation is %s\n", a->string);


         argcnt = jvm_GetMethodTypeArgumentCount(mtype);

         debugf("invokespecial: %s:%s[%u] in %s\n", mmethod, mtype, argcnt, mclass);

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
           error = JVM_ERROR_NOTOBJREF;
           debugf("object from stack is not object reference!");
           break;
         }

         /// stays the same since we poped it then placed it into locals
         //((JVMObject*)result.data)->stackCnt

         _locals[0].data = result.data;
         _locals[0].flags = result.flags;

         debugf("objref->stackCnt:%u\n", ((JVMObject*)_locals[0].data)->stackCnt);
         debugf("calling method with self.data:%x self.flags:%u\n", _locals[0].data, _locals[0].flags);

         debugf("------------------------------------------\n");
         eresult = jvm_ExecuteObjectMethod(jvm, bundle, _jclass, mmethod, mtype, locals, argcnt + 1, &result);
         free(_locals);

         if (eresult < 0) {
           error = eresult;
           debugf("propagating error down the stack frames..\n");
           break;
         }

         debugf("@@@@@@@@@@@%s\n", mtype);

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
      /// ireturn: return integer from method
      case 0xac:
      debugf("return int from method\n");
      /// return: void from method
      case 0xb1:
         if (opcode == 0xac)
         {
          /// no check if objref and decrement refcnt because it
          /// is going right back on the stack we were called from
          jvm_DebugStack(&stack);
          jvm_StackPop(&stack, _result);
         }
         ///
         debugf("scrubing stack and locals..\n");
         jvm_ScrubStack(&stack);
         debugf("here\n");
         jvm_ScrubLocals(locals);
         debugf("actually returning from method..\n");
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
        _jobject->stackCnt--;
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
              _jobject->stackCnt++;
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
        debugf("A run-time exception occured as type %i\n", error);
        return error;
      }
    }
    /// END OF ERROR MANAGEMENT
   }
  /// END OF LOOP
  return JVM_SUCCESS;
}
