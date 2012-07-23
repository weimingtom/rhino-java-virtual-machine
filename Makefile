default: ./src/*.c ./inc/*.h
	gcc -I./inc ./src/*.c ./srchead/rhino.c -o rhino
malloctestproxy: ./src/*.c ./inc/*.h
	gcc -I./inc ./src/*.c ./srchead/malloctestproxy.c -o malloctestproxy

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
