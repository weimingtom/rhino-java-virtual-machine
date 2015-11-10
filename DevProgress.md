# 2012-7-24 #
I have now started working on pulling the openJDK API into the JVM in such a manner as to test it and the VM. Also, I have now contemplated
a name change as instead use rhino as the code name of the project.

I have run into a interesting bug/problem/wall.
```
static {};
  Code:
   Stack=5, Locals=4, Args_size=0
   0:   ldc_w   #11; //class java/lang/CharacterDataLatin1
   3:   invokevirtual   #12; //Method java/lang/Class.desiredAssertionStatus:()Z
   6:   ifne    13
   ...
```
I have started trying to pull `java.lang.Character` first, and of course it leads to `java.lang.CharacterData` which leads to the above static initializer from `java.lang.CharacterDataLatin1`.

The part that has me confused is first it is making a virtual invocation of the method `desiredAssertionStatus` on the class `java.lang.Class`. Now, a virtual invocation needs a object pointer. The object pointer essentially being the special `this` pointer since it is non-static invocation. The problem is I do not understand where this pointer/object is coming from since `ldc_w` is simply loading a class reference from the constant pool:
```
const #11 = class       #86;    //  java/lang/CharacterDataLatin1
```
Which I would expect to be `java.lang.Class`, but instead it is `java.lang.CharacterDataLatin1`. So that confuses me. Also, I have not actually implemented `LDC` to deal with a class reference.

So, the first part of solving this problem is to figure out what the `LDC` opcode does exactly with a constant pool item type `CLASS` verus a `STRING` or `FLOAT`...ect.

# 2012-7-23 #
I found the Squawk project today which seems to have the ability to run on really small devices such as STM32F103ZE MCU with 64KB of RAM and 512KB of non-volatile memory. This is a reasonable target for my JVM.

I am currently focused on my object sizes in memory, locals, and stack memory consumption. Because, there are the primary culprits when it comes to memory consumption after everything has been loaded into memory.

At the moment each item on my stack consumes 12 bytes which seems quite over the top so I may very likely revise it. A 32-bit flags fields is the likely culprit, and the 64-bit data field is needed for long data types. But, I may shrink the 32-bit flags and may add support for a variable length stack item if really needed although unaligned access will have a performance hit on some architectures just not sure how much compared to interpreting.

On a architecture with 32-bit pointer each object allocation takes at minimum 26 bytes and on a 64-bit pointer architecture each takes 42 bytes which is almost double. I know there could be some tricks done to allow 32-bit pointers to be used on 64-bit machines but not sure how much of a performance hit it would be. But, for most embedded targets we are going to be talking about 32-bit word sizes so maybe not too much to worry about there.

Nevertheless, I am eager to write some more realistic programs and get a good idea of the primary memory consumption and try provide ways to lower it in a likely performance versus memory consumption trade off.

# 2012-5-28 #
My plan is to implement threads and a heap allocator. Of course the allocator should be removable in case someone provides their own implementation. I think getting a custom allocator working for embedded targets will come before actually implementing threading.

Also, I need to tidy up error reporting to make debugging Java code easier. At the moment it just spits out the opcode where the exception occurred. I think have a line number in the source would be nicer.

# 2012-5-26 #
In order to keep working I put a band-aid type fix on the tableswitch problem. Essentially, I check for an negative integer and if so I only use the last 8-bits which oddly produced 203.

# 2012-5-25 #
I have stalled on implementing the tableswitch it was working fine, untl
I ran into a weird situation. My instruction started on code offset 64 and then I got this for the fields of the tableswitch.
```
[jvm_ExecuteObjectMethod:1297] code[40]a00c30 w:a00c34
[jvm_ExecuteObjectMethod:1302] db:-117 lb:7 hb:10
[jvm_ExecuteObjectMethod:1305] map[0]:-117
[jvm_ExecuteObjectMethod:1305] map[1]:7
[jvm_ExecuteObjectMethod:1305] map[2]:10
[jvm_ExecuteObjectMethod:1305] map[3]:111
[jvm_ExecuteObjectMethod:1305] map[4]:76
[jvm_ExecuteObjectMethod:1305] map[5]:-117
[jvm_ExecuteObjectMethod:1305] map[6]:32
```

Everything adds up except the -117 which is also the default, but javap shows this:
```
   64:	tableswitch{ //7 to 10
		7: 175;
		8: 140;
		9: 203;
		10: 96;
		default: 203 }
```

# 2012-5-22 #
Got to do some thinking about threads, and a easy but powerful way to harness them. This is likely going to be non-standard with bits borrowed from the standard Java library. But, the goal is to get two threads running that can share data, and make changes later if needed.

Finally figured out a bug that has been taunting me to fix it. Essentially, the scrub stack procedure is supposed to take any items remaining on the stack and if the item is an object then decrement it's _stackCnt_. But, somehow, I had forgot to actual do that! So, while most procedures never left anything on the stack a few did and I kept having array type objects left with _stackCnt_ and unable to be garbage collected.

# 2012-5-21 #
I have finally got a stack trace printing when an uncaught exception runs back to the entry method. It looks nice despite how ugly and simple it is. One different part about it is it prints the opcode index which you would need to reference back by looking at disassembly. It is my intentions to supplement it with the usual Java source line too. So that is a project for me to tackle, but at least it has some form of a nested stack trace working!

Starting to implement protected assets with spin-locks in order to prepare for multi-threading support. The most important asset is the global object chain. I plan for this to be shared between threads and thus allowing threads to share objects. There are two main places the global object chain is used and that is in the `jvm_collect` and the `jvm_CreateObject` procedures. I do not think there is any other places but going to a nice scan of the source and see if i find any other places.

Essentially, two JVM threads can have concurrency problems but this is nature and expected. But, I hope to get the `monitorenter` and `monitorexit` working which will provide built-in locking on objects so the developer can provide a locking mechanism to protect objects that are not naturally thread-safe. Before, I do that I am going to keep working on fixing bugs and adding features and as I go along watching for contention places and protecting with a lock.

I separated the platform specific procedures, was two of them, into their own file called rhino.c. Then I left everything else in rjvm.c. This way when someone wants to work with the project they only have to alter or modify primarily the std.c, std.h, and rhino.c. Actually, rhino.c could be dropped and everything can them easily be linked against their own project or dropped into a static library.
# 2012-5-20 #
It looks like I still need to test and implement the finally clause, and synchronized. I may wait on synchronized, but I am not sure yet.

I have currently stalled on top of fully implementing exceptions. But, hopeful to be done soon.

# 2012-5-19 #
I finally got lookupswitch and tableswitch implemented. I had some initial trouble jumping over the padding bytes at first. Essentially, the table must align on an address a multiple of four. This is for performance and technical reasons. For, example the ARM7TDMI processor does not support unaligned memory access and you get better performance even on processors that can do unaligned access. For the ARM you just get a mixed up word value from the shifting system.

The _tableswitch_ opcode is interesting because you essentially index into the table, meaning the table is single value signed 32-bit offsets to be added to your opcode instruction pointer. While, the _lookupswitch_ has a table of pairs. Two 32-bit values make a pair. It works where you scan down the table looking for a match on one of the pairs. The pair is a 64-bit integer broken down into a 32-bit key and a 32-bit signed offset to be added to the instruction pointer.

At first this threw me for a loop because I was not expecting it. I found the best way to remember and understand is one is a {{table}} -> _tableswitch_ and the other is a _lookup_ -> _lookupswitch_. And, the _tableswitch_ is a table of 32-bit rows, while the _lookup_ is a table of 64-bit rows with two 32-bit values per row.

Had a small bug that prevented compiler optimizations to be turned on without a segmentation fault. It was a unintialized field that should have been zero for a linked list.

Also got some memory probing working. Here is the result for a fairly small program that calls String.format and one test function returning a constant integer value.
```
----peak memory usage---
       jvm_ExecuteObjectMethod        64
                 jvm_LoadClass      6366
             jvm_ReadWholeFile      2590
              jvm_CreateObject        64
                 jvm_StackInit        72
          jvm_AddClassToBundle       120
          jvm_MakeObjectFields        32
total-peak: 9308
----allocated memory on exit----
       jvm_ExecuteObjectMethod        32
                 jvm_LoadClass      6366
             jvm_ReadWholeFile      2590
              jvm_CreateObject        64
                 jvm_StackInit        72
          jvm_AddClassToBundle       120
          jvm_MakeObjectFields        32
```

# 2012-5-17 and 2012-5-18 #
I took some breaks from working on it, but mostly when I do I have been fixing bugs. I think the majority of features have no been implemented so I am focusing most of my work in the Java implementations which is helping to flush out the remaining bugs.

Right after I said I had most of the opcodes implemented I run into one more major one called `lookupswitch`. It is essentially a switch statement in the Java language. Its different from the other opcodes in that it is variable length. So going to take a bit to implement while being careful not to break anything.
# 2012-5-16 #
I have started the initial work of implementing the garbage collector. The design so far is fairly simple in idea and implementation. I had to fix a bug where stackCnt was being incremented and decremented via references to object fields. This behavior is incorrect as stackCnt is only intended to count the references to the stack and locals.

The garbage collector currently takes the list of objects stored in the JVM structure. It then navigates through fields that reference other objects while setting a value in the cmark field. The cmark field is incremented on each invocation of the collector so it holds a unique value. After running any object not bearing the current cmark and having a stackCnt of zero can be garbage collected.

Essentially orphaned objects that have no references to them will never have the cmark set and by the fact that we only navigate through objects in the global JVM chain is they have a stackCnt greater than zero.

I still need to add support for navigating object array fields, and being able to determine if an object is an object array, primitive array, or object.

# 2012-5-15 #
Still working on the test-case infrastructure. At the moment it is almost working, but a few bugs to iron out. Essentially, since my VM at the moment starts differently doing test-case work is not as straight forward. Currently, you have to specify to the VM which classes to load and which one has to proper signature. The VM should look for `public static void main(String[])`, but because of the way I was building it I create an instance of the object then call a `int main()` method instead. I think later I will work on getting it to call the appropriate function for entry.

I also have managed to get a test-build system going. Essentially, it run a series of tests and checks that the output from my VM is the same as the stock VM. This should help to catch some problems earlier on. It is also a place to put bug tests at when I find bugs.

To use the test build system you need Python3.x installed under `/usr/bin/python3.1}}, or just change the header line in the {{{test.py` script.
```
./test.py
```
After typing `./test.py` the testing phase should begin. You will see individual tests being compiled (if needed) and performed then the result of a fail or pass.

The test bed of tests has helped greatly. I discovered a many operand reversal problems in my math operations, and it helps me to see that I have been correctly implementing integer math.
# 2012-5-14 #

Below completed:

~~I got at least two ways to do classes with a native code backing.~~

~~(_1_) I can produce PIC code using the bin-utils suite (gcc/ld/objcopy), which could then be inserted into a generated special class file. Essentially, embedding the native instructions somewhere in it and then a little support on the JVM side to detect this and call the embedded code.~~

(_2_) Have the target classes loaded as phantom classes. Essentially, I create a class but give it a hollow implementation. The good sides to this is that I could in theory mix Java and native code in the class. I just have to mark the native methods with some attribute? So, this is currently my favorite because it should provide flexibility and it still easy to implement. It would work sort of like how the java.lang.String works, but more to that effect. Since all java.lang.String methods are Java. It is only the creation of a string object that differs.

~~Also, I have run into a mental block on how best to handle static fields and methods. I will later find a solution, but for now some testing is needed to ensure I understand the entire problem.~~
I stored them in the JVMClass structure since that really is the only static part of the JVM when it comes to objects.

~~I have realized after a little testing that I essentially need to store the static fields in my JVMClass object, since that is true static structure throughout the VM. The VM uses two special instructions for accessing the static fields anyway so it should not be too hard to work with.~~
I did it.

~~Have run into a slight wall trying to find where the static variable initialized value is. For example:~~
```
class System {
    static int    x = 4;
}
```
~~I can setup for _x_ to be static, but can not find the value say _4_ in this case.~~

I finally found where a non-final is initialized and it is using a special method emitted by the compiler as, 

&lt;clinit&gt;

 with type ()V. Once this method is executed the static fields will be initialized.

But, after all that I am still bugged with my flag system for defining the primitive or basic object type of a variable. You see anything I push onto the stack that is a integer such a short is automatically widened to being a uintptr (64-bits). But, it still has a flag saying that it is only a short, and if I am looking to do a type check I will fail it because it is a short and not an int for instance -- actually, now that I wrote that I need to go back and change my stacks and such to uint int64 because uintptr will only be 32-bit on 32-bit platforms -- oops.

I looked at my integer arithmetic operations and I am still worry some over there being a different in my VM and another. I am thinking just to let it go for now and fix it later.

# 2012-5-13 #
Had a let down today. Ran a test with a loop and my stock JVM beat me by a lot. Here is the test case:
```
class Test {

  public Test() {
  }

  public int main() {
    String      a;
    String      b;
    int         x;
    int         z;

    a = "who me?";
    b = "who you";

    z = 0;
    for (x = 0; x < 1000000; ++x)
      z += a.compareTo(b) + x;

    return z;
  }

  static public void main(String[] args) {
    Test        test;
    test = new Test();
    System.out.print(test.main());
  }
}
```

My JVM took about 35 seconds, and the stock took 0.151 seconds. So apparently it is doing something differently in the handling of the loop and/or java.lang.String. I do have a java implementation of string. So I may try leaving out string and having just the loop.

I did a few other tests and still my JVM seems to be lacking. I suppose this could have something to do with the fact that my stock JVM is doing JIT compilation.

I worked some more on java.lang.String. Got the compareTo method worked in. Then started doing more work and testing on constructors. I discover a fairly nasty bug where before calling a method the argument count would count two instead of one for every array. Once fixed the String constructor appeared to be called.

Also, I have started looking at and thinking more about exposing a System namespace. So I can implement _stdout_ and be able to generate texual debugging information from with in the java class. The problem is that I in some decent way must interface directly from Java in my C code so the C code can write to stdout and such. I can do it I am just leaning on taking my time and letting good code sense ideas float around.

# 2012-5-12 #
I have finally squashed a long standing bug with the _stackCnt_ field for objects. Essentially, what I was doing was keeping track of how many separate instances existed on the _stack_ , _fields_ , and _locals_ . This tells me that the object is still referenced in one of the three if the object's _stackCnt_ was greater than zero. The problem is stuff was fairly messy and somewhere along the way the count was getting off in both directions. My mistake was that I had been trying to do it by hand where as once something popped from the stack I would write code to adjust it's _stackCnt_, but thats really messy. So I integrated the _stackCnt_ logic into the _StackPush_ and _StackPop_ functions. Also, I created a new function for setting a local variable which manages the _stackCnt_ for me. The only place in the code where _stackCnt_ is handled manually sort of speak is in the _getfield_ and _putfield_ opcode blocks.

Also, added a number of missing opcodes. Going to try to do a little performance test later on just for fun. I want my _VM_ to be fast because thats important too. So after doing my test I will have an idea of how slow or fast my _VM_ implementation is so far even thought it is not really fair since I do not have everything implemented yet -- like I said just for fun though.

The test results for:
```
class Test extends Apple {
  Object        a;

  public Test() {
  }

  public int meme() {
    return 11;
  }
  public int main() {
    int x;
    int y;

    for (y = 0; y < 5000; ++y);

    return y;
  }

  static public void main(String[] args) {
    Test        test;
    test = new Test();
    test.main();
  }
}
```

The results:
```
$ java -version
java version "1.6.0_18"
OpenJDK Runtime Environment (IcedTea6 1.8.13) (6b18-1.8.13-0+squeeze1)
OpenJDK 64-Bit Server VM (build 14.0-b16, mixed mode)

$ time java Test

real	0m0.154s
user	0m0.024s
sys	0m0.104s

$ gcc *.c -o rhino && time ./rhino

real	0m0.005s
user	0m0.004s
sys	0m0.000s
```

Which, looks good. As in if my implementation was slower than I had done something very wrong. But, now I have a good overlook at the gap currently between _Rhino_ and the behemoth of my standard JVM.

Also, I have started fully planning the garbage collection. Already I have the basic design of how to link refereeing objects to one another. The only part I need to continue to design out is the when garbage collection will occur and the exact method to navigate my object reference system.

Squashed a bug in special/virtual execute method where it was not handing the correct locals array. It was handing off the locals array for the caller. Also, got java.lang.String to report it's string length which is greater news since now java.lang.String is boot strapped in so if I wanted I could start adding in functions for it.

I still need to work on garbage collection more, and finish implementing more opcodes.

# 2012-5-11 #
Been working on the opcodes getfield and setfield. On intial design I did not realize that the fields are referenced by a string so therefore it has taken a slight bit longer than wanted to implement fields on objects. The create object function is a little slower now since I have to run backwards through super classes to determine all the fields that are needed. I could however cache this and copy which would be a big performance booster, but at the moment trying to just make it work and not worry about performance.

Also on my mind is _not_ implementing multi-dimensional arrays. There are a tad harder to implement because you got extra re-direction. Also you can do anything with a single dimensional array as you can with a multi. So they are not actually needed. They can be helpful at times, but you can if you like create the exact same thing with singles. However, multi support _might be faster with a in machine implementation_! So there may be a reason to implement them after all. But, I need to study how to implement them more because I actually do. I know some tiny implementations of Java do not support multi arrays either so mine wont be the only one. I do expect to implement them one day just not now.

Got some problems with my reference count for objects. Essentially, I track two things for garbage collection. I track the number of references of the object between the stack and locals. Then, I track through _getfield_ and _putfield_ the actual references between objects. So if I store object _a_ in a field in object _b_. Then object a and b share a reference which means as long as one of them has a _stackCnt_ above zero they are safe from garbage collection.. However, my _stackCnt_ code has got out of sync somewhere and it is not being properly decremented.

# 2012-5-10 #
I have been making decent progress in instruction addition. There were some slow downs when dealing with native arrays and object reference arrays. Also, no memory bugs can be found yet which makes me feel good about the code base so far. Aiming for a slow but steady development of it right now. I am excited about it each time I implement something else and see it working.

I still need to implement the infrastructure to easily implement native classes for tricky cases such as _java.lang.String_ and friends, or if
possible I would not mind implementing as much as I can for _java.lang.String_, but right now I am having to figure out how to implement it all in a good way.

Also on my mind is potentially using the processor to remove a lot of checks in order to speed up execution time for bundles that have proven to be stable or from a known source or good compiler. Including array bounds checking. That way once everything has been tested you can move to a more performance level by defining a macro which would remove a lot of the
checks.