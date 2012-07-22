default:
	gcc -Os exec.c ms.c rhino.c rjvm.c stack.c std.c -o rhino
malloctestproxy: ms.c malloctestproxy.c rjvm.c stack.c std.c
	gcc -0s exec.c ms.c malloctestproxy.c rjvm.c stack.c std.c -o malloctestproxy

java: makejavalang makecore maketest makeutil

maketest: ./Test.java
	javac ./Test.java
makejavalang: ./java/lang/*.java
	javac $?
makecore: ./Core/*.java
	javac $?
makeutil: ./Util/*.java
	javac $?

clean:
	rm ./java/lang/*.class
	rm ./Core/*.class
	rm *.class
