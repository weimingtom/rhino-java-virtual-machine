default: makejavalang makecore maketest
	gcc -Os *.c -o rhino

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