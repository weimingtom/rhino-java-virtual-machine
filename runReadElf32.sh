#!/bin/sh
javac ./readelf32/Main.java &&
make &&
./rhino ./readelf32/Main.class ./java/lang/Object.class ./java/lang/Array.class ./java/lang/Exception.class ./java/lang/Integer.class ./java/lang/ExceptionStackItem.class ./java/lang/String.class ./Core/Thread.class ./Util/MemoryStream.class :Main