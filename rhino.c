#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rjvm.h"
#include "ms.h"

int jvm_core_core_handler(struct _JVM *jvm, struct _JVMBundle *bundle, struct _JVMClass *jclass,
                               uint8 *method8, uint8 *type8, JVMLocal *locals,
                               int localCnt, JVMLocal *result) {
  int                   x;
  int                   c;
  int                   error;
  JVMBundleClass        *cbc;
  JVMObject             *jobject;
  JVMObject             *sobject;
  JVMObject             *pobject;
  uint8                 *cn;
  uint8                 lbuf[128];
  uint8                 *buf;
  FILE                  *fp;
  struct stat           _stat;
  JVMObject             *_jobject;
  
  debugf("success:%s:%s\n", method8, type8);
  // determine what is being called
  for (x = 0, c = 0; method8[x] != 0; ++x)
    c += method8[x];

  debugf("native:%s:%x\n", method8, c);
  //jvm_exit(-5);
  switch (c) {
    // ReadFile
    case 0x2fc:
      sobject = (JVMObject*)locals[0].data;
      debugf("sobject %x\n", sobject);
      if (!sobject)
        break;
      pobject = (JVMObject*)sobject->_fields[0].value;
      for (c = 0; c < pobject->fieldCnt; ++c)
        lbuf[c] = ((uint8*)pobject->fields)[c];
      lbuf[c] = 0;

      fp = fopen(&lbuf[0], "rb");
      fstat(fileno(fp), &_stat);
      c = _stat.st_size;
      buf = (uint8*)jvm_malloc(sizeof(uint8) * c);
      fread(&buf[0], c, 1, fp);
      fclose(fp);

      error = jvm_CreatePrimArray(jvm, bundle, JVM_ATYPE_BYTE, c, &_jobject, buf);
      if (error)
        return error;
      result->data = (uint64)_jobject;
      result->flags = JVM_STACK_ISARRAYREF | JVM_STACK_ISOBJECTREF | JVM_STACK_ISBYTE;
      break;
    // Exit
    case 0x19a:
      jvm_exit(-1);
      break;
    // PrintString
    case 0x484:
      sobject = (JVMObject*)locals[0].data;
      debugf("sobject %x\n", sobject);
      if (!sobject)
        break;
      pobject = (JVMObject*)sobject->_fields[0].value;
      debugf("pobject %x %u\n", pobject, sobject->fieldCnt);
      if (!pobject)
        break;

      debugf("printc:");
      for (c = 0; c < pobject->fieldCnt; ++c) {
        jvm_printf("%c", ((uint8*)pobject->fields)[c]);
      }
      jvm_printf("\n");
      break;
    // EnumClasses
    case 0x463:
      // get count of classes
      for (c = 0, cbc = bundle->first; cbc != 0; cbc = cbc->next, ++c);
      error = jvm_CreateObjectArray(jvm, bundle, "java/lang/String", c, &jobject);
      if (error) {
        return error;
      }
      for (c = 0, cbc = bundle->first; cbc != 0; cbc = cbc->next, ++c) {
        // create string object
        error = jvm_CreateObject(jvm, bundle, "java/lang/String", &sobject);
        if (error)
          return error;
        // get string we need to put in it
        cn = jvm_GetClassNameFromClass(cbc->jclass);
        // get length of that string
        for (x = 0; cn[x] != 0; ++x);
        // create primitive array to hold string
        error = jvm_CreatePrimArray(jvm, bundle, JVM_ATYPE_BYTE, x, &pobject, 0);
        // fill primitive array
        for (x = 0; cn[x] != 0; ++x)
          ((uint8*)pobject->fields)[x] = cn[x];
        if (error)
           return error;
        // add primitive array to String field
        sobject->_fields[0].value = (uintptr)pobject;
        sobject->_fields[0].jclass = 0;
        sobject->_fields[0].aflags = JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF;
        // add to object array of String
        jobject->fields[c] = (uint64)sobject;
        //jvm_exit(-5);
      }
      // return the object array of String
      result->data = (uintptr)jobject;
      result->flags = JVM_STACK_ISOBJECTREF | JVM_STACK_ISARRAYREF;
      return JVM_SUCCESS;
  }
  return JVM_SUCCESS;
}

int main(int argc, char *argv[])
{
  uint8                 *buf;
  JVMMemoryStream       m;
  JVMClass              *jclass;
  JVMBundle             jbundle;
  JVMBundleClass        *jbclass;
  JVM                   jvm;
  JVMObject             *jobject;
  JVMLocal              locals[10];
  uint32                size;
  int                   result;
  JVMLocal              jvm_result;
  JVMLocal              _result;
  int                   x;
  uint8                 *entryClass;
  uint8                 *utf8;

  jvm.objects = 0;
  jvm.cmark = 0;
  jvm.mutex = 0;
  jbundle.first = 0;
  jvm.bundle = &jbundle;

  //x = jvm_GetMethodTypeArgumentCount("(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
  //jvm_printf("argcnt:%u\n", x);

  buf = jvm_ReadWholeFile("./Core/Core.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jclass->flags = JVM_CLASS_NATIVE;
  jclass->nhand = jvm_core_core_handler;
  jvm_free(buf);
  jvm_AddClassToBundle(&jbundle, jclass);

  for (x = 1; x < argc; ++x) {
    if (argv[x][0] == ':') {
      // holds classpath and class name for entry
      entryClass = &argv[x][1];
    } else {
      debugf("@@>loading %s\n", argv[x]);
      buf = jvm_ReadWholeFile(argv[x], &size);
      msWrap(&m, buf, size);
      jclass = jvm_LoadClass(&m);
      jvm_free(buf);
      debugf("@@>jclass: %lx\n", jclass);
      jvm_AddClassToBundle(&jbundle, jclass);
    }
  }

  // make static fields for all classes in bundle,
  // also this calls the special <clinit>:()V method
  debugf("here\n");
  jvm_MakeStaticFieldsOnBundle(&jvm, &jbundle);
  debugf("here\n");
  /// create initial object
  result = jvm_CreateObject(&jvm, &jbundle, entryClass, &jobject);
  jclass = jvm_FindClassInBundle(&jbundle, entryClass);

  if (!jobject) {
    debugf("could not create object?\n");
    jvm_exit(-1);
  }

  jobject->stackCnt = 1;
  locals[0].data = (uint64)jobject;
  locals[0].flags = JVM_STACK_ISOBJECTREF;
  jvm_result.data = 0;
  jvm_result.flags = 0;
  result = jvm_ExecuteObjectMethod(&jvm, &jbundle, jclass, "main", "()J", &locals[0], 1, &jvm_result);
  if (result < 0) {
    if (!jvm_result.data) {
      debugf("error occured too soon; error-code:%i\n", result);
      exit(-1);
    }
    // the exception should be stored in jvm_result
    debugf("exception code:%i\n", result);
    debugf("jvm_result.data:%x jvm_result.flags:%x\n", jvm_result.data, jvm_result.flags);
    // walk the stack
    errorf("-------- UNCAUGHT EXCEPTION ---------\n");
    errorf("  %s\n", jvm_GetClassNameFromClass(((JVMObject*)jvm_result.data)->class));
    jvm_GetField((JVMObject*)jvm_result.data, "msg", &_result);
    debugf("here %x\n", _result.data);
    jvm_GetString((JVMObject*)_result.data, &utf8);
    errorf(" msg:%s\n", utf8);

    result = jvm_GetField((JVMObject*)jvm_result.data, "first", &_result);
    while (_result.data != 0) {
      result = jvm_GetField((JVMObject*)_result.data, "methodName", &jvm_result);
      jvm_GetString((JVMObject*)jvm_result.data, &utf8);
      errorf("    method:%s", utf8);
      result = jvm_GetField((JVMObject*)_result.data, "className", &jvm_result);
      jvm_GetString((JVMObject*)jvm_result.data, &utf8);
      errorf(" class:%s", utf8);
      result = jvm_GetField((JVMObject*)_result.data, "methodType", &jvm_result);
      jvm_GetString((JVMObject*)jvm_result.data, &utf8);
      errorf(" type:%s", utf8);
      result = jvm_GetField((JVMObject*)_result.data, "opcodeIndex", &jvm_result);
      errorf(" opcode:%u\n", jvm_result.data);
      //result = jvm_GetField((JVMObject*)_result.data, "sourceLine", &jvm_result);
      //debugf("sourceLine:%u\n", jvm_result.data);
      // get next item, if any
      debugf("    -----\n");
      result = jvm_GetField((JVMObject*)_result.data, "next", &_result);
    }
    debugf("calling collect.. %x\n", jobject);
    jvm_collect(&jvm);
    return -1;
  }

  jvm_printf("done! result.data:%i result.flags:%u\n", jvm_result.data, jvm_result.flags);

  jvm_collect(&jvm);

  return 1;
}
