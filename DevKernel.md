The JVM was designed from the beginning to be used on a embedded platform. In order to do this structures, interfaces, and methods must be designed.

This page is going to be the collection of ideas and concepts that will allow as much as possible to be created in Java. I hope despite the speed disadvantage I will be able to extend the proof of concept to even handling interrupt routines.


```
// for threads to use this they must all have compiled against it
// during compile time, or a mock up of it at the very least
class Message {
	String		message;
}

class RecvMessage {
	String		fromRPA;
	Object		object;
}

class Core {
	public static native int LoadClass(long address);
	public static native String[] EnumClasses();
	public static native Object CreateObject(String className);
	public static native int IPCSendMessage(String toRPA, Object object);
	public static native Object IPCReadMessage();
	public static native void CollectGarbage();
	public static native void MemoryAdd(long from, long size);
}

class VirtualFileSysem {
	public VirtualFileSystem() {
		// enum classes and look for VFS drivers
	}
}

class Kernel {

	void entry() {
		String[]	classes;
		classes = Core.EnumClasses()
		// need method to dynamicaly call a method on a class	
	}
}
```