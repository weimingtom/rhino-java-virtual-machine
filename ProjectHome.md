# Overview #
The goal is to have a very small package for and tuned to a wide range of similar architectures primarily such as ARM, X86, X86-64, and any other which can be feasibly targeted.

(1) _Also for study of a Java virtual machine._

(2) _The codename Rhino is a pun at fact that the projects aims to be very small for embedded devices._

(3) _The code at the moment is ALPHA._

(4) _A minimal usage of the standard library is a requirement. I want this package to be able to work on embedded machines which have no standard library and thus what functions I do use are trivial to implement or patch. My file reading code is all in one place. The JVM has whats called a bundle which are all the classes loaded into memory._

(5) _I would like to implement a JIT compiler for this JVM, but at the moment it is a very low priority because I need to finish the VM implementation to have a better understanding before I integrate a JIT into the VM._

(6) _You should have Python 3.x and Make installed, but it is not required. To build with out Make installed simply compile and link all source files. The test-case system can not be run with out Python 3.x, but it is also optional to run it. I use it to help find bugs and inconsistencies._

# Project State #
The project state is currently alpha due limited testing between features things could break here and there. I am almost done implementing all the core required features. There are only a handful of opcodes not yet implemented or at least non-important ones.

I have recently started to implement the java.lang.String, and have now the ability to call `void Core.Core.PrintString(String)`. But, the rhino executable currently has tons of debug output unless you disable it using the `debugf` macro.

There is no official support or workings for the standard library of Java primitives. So what you get is the bare metal implementation of Java at the moment. This is likely not what you are looking for, but again it could be.

The code is fairly organized and small. This means you can dive into it and figure it out fairly easy. There are some parts that you will be staring at for some time, but with a little effort you will understand the code with no major problems.

# Known/Partial/Missing Features #
```
Missing    Synchronized                - not implemented yet
                                         Working on getting this implemented.
Missing    Threading                   - not implemented in the embedded run-time yet
Missing    Multi-Dimension Arrays      - not anytime too soon
                                         They are not used that much, but I may implement
                                         one day.
Missing    Async Garbage Collection    - missing due to threads missing, but support
                                         is in place
Working    Sync Garbage  Collection
Missing    Standard Java Classes       - missing; trying to implement at least a 
                                         subset of the standard Java classes

Partial    Exception Stack Trace       - currently this is partially working; as in it
                                         does have the infrastructure and does print one
                                         if a uncaught exception occurs but it is not
                                         as nice and pretty as I want it
```

# Memory Usage #
From some rough tests it looks like it going to take 9K just to load a few classes, then your potentially looking at 50k to 300k in run-time memory usage for some minimal applications.

Each object on a 64-bit pointer platform is going to be 39 bytes, and on a 32-bit platform 23 bytes at minimum. That is not counting fields. Padding will likely increase it another 0 to 3 bytes.

The results are out for a simple program calling `String.format` and then a test function (very small program). These are the memory statistics:
```
----peak memory usage---
       jvm_ExecuteObjectMethod        64
                 jvm_LoadClass      6366
             jvm_ReadWholeFile      1472
              jvm_CreateObject        32
                 jvm_StackInit        12
          jvm_AddClassToBundle       120
          jvm_MakeObjectFields         0
summation-peak: 8066
actual-peak: 7934
----allocated memory on exit----
       jvm_ExecuteObjectMethod        64
                 jvm_LoadClass      6366
             jvm_ReadWholeFile         0
              jvm_CreateObject        32
                 jvm_StackInit        12
          jvm_AddClassToBundle       120
          jvm_MakeObjectFields         0
```
Also, one must consider that the `jvm_LoadClass` is a one time only punch, unless you load more classes after the initial ones.

The program (see Core/Core.java and java/lang/String.java for details):
```
class Test {
  public int main() {
    //String[]            sa;
    //String              s;
    //sa = Core.Core.EnumClasses();
    //s = sa[0].concat(sa[1]);

    Core.Core.PrintString(String.format("a%sb%%c", "hello world"));

    //return toot("hello world", sa[0], sa[1]);
    return String.test(99, 88);
  }
}
```

I had to make some adjustments. I was initially allocating for 256 locals, and 1024 stack items. But, once I realized where it was specified for the maxLocals and maxStack then the memory usage came down to what it is now.

# Size #
The current output size is between _17k_ to _30k_ in bytes in ELF32 format with debugging messages disabled. With debugging messages enabled it is between 25k and 40k. I left room for expansion with out having to update my numbers. But, it is fairly small just like I intended it to be. This is also targeting the x86\_64 architecture.

# No STDLIB #
If you wish you can compile and implement the VM with out the standard C library. This is done by editing the files _std.c_ and _std.h_ to your liking. Only a small handful of standard calls are used in the entire VM.

Also, the function `jvm_ReadWholeFile` used in `rhino.c` can be reimplemented,
but you are likely going to need to remove or make over `rhino.c` completely since
it is simply designed to facilitate the operation of rhino in a console type environment.

Here is a brief overview of the standard C library calls that I use (std.h):
```
void jvm_free(void *p);
#define jvm_malloc(s) jvm__malloc((s), __FUNCTION__, __LINE__);
void *jvm__malloc(uintptr size, const char *function, uint32 line);
//void jvm_printf(const char *fmt, ...);
#define jvm_printf printf
void jvm_exit(int result);
int jvm_strcmp(const char *a, const char *b);
int jvm_strlen(const char *a);
```

The jvm\_malloc and jvm\_free functions actually support an internal malloc/free implementation independent of the standard library. To enable this simply
edit `conf.h` and uncomment the `INTERNALMALLOC` directive.

# Testing #
See the README section (below).

### README ###
This is the README copied. The actual README may be more updated.
```
Rhino

This is a project that I started to help me learn more about the 
internal workings of a Java virtual machine, and also to produce
a usable virtual machine for a embedded platform.

== COMPILATION ==
To compile the virtual machine you need to execute 'make' in the working
directory of the souce which should be the base directory. This will
create a executable [rhino] which can be executed.

== READCLASS Sub-Directory ==
This contains a java program using my custom standard library of 
functions to load a class like it's self and parse it. This is just
a simple demonstration of doing something.

== MALLOCTESTPROXY ==
The malloc test proxy is named because a stub is generated that acts
like a proxy to the internal malloc routines. The proxy is an executable
which can be generated by invoking 'make malloctestproxy' which produces
an executacle named [malloctestproxy]. The front-end to this proxy is a 
Python script called [malloctestproxy.py]. The Python script will 
execute the proxy generated and perform a bunch of random allocations,
deallocations, writes, and reads. It verifies that written data matches
when it is read many operations later. It essentially tries to cause the
malloc implementation to fail and corrupt it's self.

Also, the conf.h file should have the INTERNALMALLOC directive uncommented,
or otherwise you will end up testing the standard library malloc/free
implementation. 

This provides a good standard for testing any changes to the malloc
implementation. Since any bugs present when this is used will simply
manifest as other problems and become highly difficult to track.

== QUICK DEMO ==
To perform a quick demo and provide a starting point execute these
commands while in the root project directory.

make test1
./rhino ./ert/java/lang/Object.class 
        ./ert/java/lang/ExceptionStackItem.class 
        ./ert/java/lang/Exception.class 
        ./ert/java/lang/String.class 
        ./ert/java/lang/Array.class 
        ./tests/test1/Main.class :Main

You should place everything for the './rhino' command on a single line. 
Also take note how all classes must be specified. The 'rhino' binary
will not perform a disk search to find missing classes and expects
you to specify all needed classes. This may seem non-standard and it is,
but if you wished you could implement automatic loading and searching
quick easily I just have not had the time to do so yet. And, if you do
you need an option to specify the class path. 

Th ERT directory stands for embedded run-time and is seperate from the
standard such as openJDK. I wish to one day be able to support the 
openJDK, but for now I am opted to work on a much smaller and specialed
subset just for small embedded platforms.

== SOUCE AND HEADER FILES OVERVIEW ==

=== SRC AND INC DIRECTORY ===
	ms.c & ms.h
		The memory stream implementation simply provides a ease of use
		to take a chunk of memory and treat it like a stream of primitive
		types. This is very similar to the [stack.c] and [stack.h] 
		implementation.

	port.h
		This contains all the primitive type defines. They are stored in
		one file to make cross-platform changes easier to make.

	conf.h
		This simply stores any pre-processor directives that need to be
		global. Or, end up being global.

	rjvm.h & rjvm.c
		All the utility functions (or almost all) are kept in these two
		files. Also, most of the primary data structures for the VM are
		defined in the header. These are the heart of the VM, except for
		the actual interpretor which is stored in [exec.c] and [exec.h].

	exec.c & exec.h
		This is the interpretor core. It holds the primary routine for
		the interpretor. It makes use of various other sources and headers.

	stack.c & stack.h
		Both these files hold a implementation of a basic stack which 
		supports the push, pop, discardTop, and a debug routine.

	std.c & std.h
		These two files hold all the function call that would normally
		land in the standard runtime or library. This provides an easy
		way for the embedded developer to hook, emulate, or replace this
		functions. A minimal of standard calls are used in the JVM, but
		the ones that are used are placed in these files.

	rmalloc.c & rmalloc.h
		These hold the internal implementation of a malloc/free. They are
		called by the [std.c] implementation based on if a pre-processor
		directive has been set known as 'INTERNALMALLOC'. Only the [std.c]
		source file need be compiled with this directive. Although, from
		the time of this writting all source files are compiled with the
		directive which is set in [conf.h].
		
	conf.h
		Currently, only home to the pre-processor directive INTERNALMALLOC
=== SRCHEAD DIRECTORY ===
As implied this is the directory for heads to the JVM. A head is essentially
a frontend. It brings everything together and provides initialization and 
configuration. For example 'rhino.c' provides a command line head, while
'malloctestproxy.c' provides a command line proxy to the malloc and free
implementation. While, another head might provide a ARM bootstrap or 
something for an embedded system.

	rhino.c
		This holds the entry function. It prepares to interpret a Java
		class file. It also handles parsing command line arguments, and
		loading of class files into a bundle. 
		
		If you are going to embed or use the rhino project then this file
		is the one you will either edit the most or simply replace. It 
		currently provides testing from the command line.

	malloctestproxy.c
		Provides a command line interface through stdout and stdin pipes
		to test the malloc and free implemention. See 'malloctestproxy.py'.
```


# Compile-Time Embed, Compile-Time Link, Or Run-Time Load #
You are at the point where you are looking to use Rhino, but your not sure how. You have many options with 3 being major options.

The good news is Rhino can be used in your project multiple ways. The embed method may offer you a more simple approach with the link method for second. If you compile or link in you can of course call the method just like they are a part of your source code.

### Embed ###
Simply copy or link the source and header files into your project. Using this method you will need to remove `main` from rhino.c, and tag into it using your own implementation.

### Link ###
Create a static library and link it against your project while using Rhino's header files. This is essentially the same as Embed except Rhino stays in it's own lib file.

### Load ###
You could create a shared library which can be loaded at run-time and linked into using a variety of methods.