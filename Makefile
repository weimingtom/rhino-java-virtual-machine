default:
	gcc -Os *.c -o rhino

java: makejavalang makecore maketest

maketest: ./Test.java
	javac ./Test.java
makejavalang: ./java/lang/*.java
	javac $?
makecore: ./Core/*.java
	javac $?

clean:
	rm ./java/lang/*.class
	rm ./Core/*.class
	rm *.class