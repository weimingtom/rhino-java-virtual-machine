package java.lang;
// This serves as a phantom class. It is just here to allow the
// compilation of classes that reference this class. But, it's
// entire implementation is built-in to the JVM.
public class System {
  public System() {}

  public native int Boogey(int x);
  public native void WriteConsole(String what);
}