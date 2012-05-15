package java.lang;
// This serves as a phantom class. It is just here to allow the
// compilation of classes that reference this class. But, it's
// entire implementation is built-in to the JVM.
public class System {
  static int            x = -1234;

  public System() {
  }

  public static int test(int y) {
    return x + y;
  }

  public static native void WriteConsole(String what);
}