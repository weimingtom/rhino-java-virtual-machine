To call rhino from the command line you must, at this current time, specify the classes needed to be loaded into memory. Then you must specify the actual class with a main method in it.
```
class MyClass {
  public int main() {
    return 4;
  }
}
```

So for the command line you would do:
```
  ./rhino ./java/lang/Object.class ./MyClass.class :MyClass
```

This tells the loader in rhino to load two classes. The Object class is required. Then, it tells it to find the `public int main()` method in the class `MyClass`. It also creates the Object for MyClass in memory, before calling the main method.

I understand this is not a standard way, but since the code base was designed with an embedded system in mind this makes more sense. On an embedded system you would append all the classes that need to be loaded onto your kernel image. Then scan them down and load and finally enter into a class by creating an Object and calling a specific method. I have only simply extended this mechanism onto the command line for testing reasons.