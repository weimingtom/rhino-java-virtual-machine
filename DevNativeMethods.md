This page will cover how to implement one or more native methods. At the moment only one standard way is presented. It also requires that the native code be compiled into the VM image. Hopefully, at some later time I can give at least some support to external image loading from a memory buffer much like that which is used to load classes into the VM bundle.

For each class you wish to implement native methods in you must do two things. First, you must create a class by compiling a java source file. Here is an example with one native function.

```
public class System {
  public System() {
  }

  public static native void WriteConsole(String what);
}
```

You may mix native and java methods together in the class the only difference is with native methods you specify no code and just terminate it as a statement just like above.

Once you have your class file you also need to add a function just like this one into the source. You should use a unique name that reflects the class you created to keep things organized.

```
int jvm_system_handler(struct _JVM *jvm, struct _JVMBundle *bundle, struct _JVMClass *jclass,
                               uint8 *method8, uint8 *type8, JVMLocal *locals,
                               int localCnt, JVMLocal *result) {
  
  debugf("success:%s:%s\n", method8, type8);
  //result->data = 800;
  //result->flags = JVM_STACK_ISINT;
  return 1;
}
```

This will be associated with the class your implementing native methods in like so:

```
  buf = jvm_ReadWholeFile("./java/lang/System.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  jclass->flags = JVM_CLASS_NATIVE;
  jclass->nhand = jvm_system_handler;
  jvm_AddClassToBundle(&jbundle, jclass);
```

A normal class load will not alter the jclass->flags or jclass->nhand. The flags must specify at the very least JVM\_CLASS\_NATIVE. The _nhand_ field will point to your function your implemented.

Now, you have the ability to fully implement your methods. You will notice that the _method8_ and _type8_ parameters are strings. They shall allow you to determine which native method is being called. If you only have a single native method you could in theory ignore those, but if you have more you must implement a way to determine the difference between calls. If possible you could even check a single byte that you know to be different between all of your native methods.

