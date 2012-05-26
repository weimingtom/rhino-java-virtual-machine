#!/bin/sh
javac ./readclass/Main.java &&
make &&
./rhino ./readclass/JVMPoolNameAndType.class ./readclass/JVMPoolUtf8.class ./readclass/JVMPoolFieldRef.class ./readclass/JVMPoolClass.class ./readclass/JVMPoolString.class ./readclass/JVMPoolMethodRef.class ./readclass/Main.class ./java/lang/Object.class ./java/lang/Array.class ./java/lang/Exception.class ./java/lang/Integer.class ./java/lang/ExceptionStackItem.class ./java/lang/String.class ./Core/Thread.class ./Util/MemoryStream.class :Main