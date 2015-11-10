
```
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
```

There are more constant pool types, but these are the major ones. All constant pool items have a single byte as their type. It is the first
byte.

I have derived specific structures from a generic structure.
```
typedef struct _JVMConstPoolItem {
  uint8                 type;
} JVMConstPoolItem;
```

Also, now let us take a look at the JVMClass object.
```
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
```

The _pool_ field is how you can reference into the constant pool for any _class_ object. It is an pointer to an array of pointers of the type I showed you above. All you can access is the type field. For example:
```
  buf = jvm_ReadWholeFile("Peach.class", &size);
  msWrap(&m, buf, size);
  jclass = jvm_LoadClass(&m);
  debugf("type:%u\n", jclass->pool[0]->type);
```

This will display the type of entry at that pool index. But, if it is a specific type. Let us pretend that the type is TAG\_CLASSINFO=7.
```
  JVMConstPoolClassInfo         *c;
  JVMConstPoolUtf8              *a;
  c = (JVMConstPoolClassInfo*)jclass->pool[0];
  a = (JVMConstPoolUtf8*)jclass->pool[c->nameIndex - 1];
  debugf("a->string:%s\n", a->string);
```
Now, we have accessed the string attribute of a JVMConstPoolUtf8, and we got our _Utf8_ from a JVMConstPoolClassInfo. You should note that any reference  in _java bytecode_ to a constant pool item is one too high. This is because I used a zero based index for loading in the constant pool, but a _bytecode_ reference of 1 is actually a 0. So always subtract one from any _bytecode_ reference into the constant pool.

If your coding and your not sure what type you are working with then use a simple debug statement to display the type.

## FieldRef Chart ##
```
JVMConstPoolFieldRef --> classIndex --> JVMConstPoolUtf8 --> string
                     --> nameAndTypeIndex --> JVMConstPoolNameAndType --> nameIndex -> JVMConstPoolUtf8 --> string (field name string)
                                                --> descIndex -> JVMConstPoolUtf8 --> string (field type string)
```