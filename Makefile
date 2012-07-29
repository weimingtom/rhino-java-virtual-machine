default: ./src/*.c ./inc/*.h
	gcc -I./inc ./src/*.c ./srchead/rhino.c -o rhino
malloctestproxy: ./src/*.c ./inc/*.h
	gcc -I./inc ./src/*.c ./srchead/malloctestproxy.c -o malloctestproxy

ert.done: ./ert/sys/*.java ./ert/java/lang/*.java
	javac ./ert/sys/*.java -classpath ./ert/
	javac ./ert/java/lang/*.java -classpath ./ert/
	echo 1 > ert.done

test1: ert.done
	javac ./tests/test1/*.java -classpath ./ert/

maketest: ./Test.java java
	javac ./Test.java
	
clean:
	rm ./java/lang/*.class
	rm ./Core/*.class
	rm *.class
