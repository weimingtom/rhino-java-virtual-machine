package java.lang;

public class Object {
  public Object() {
  }
  
  public final native Class getClass();
  public native int hashCode();
  public boolean equals(Object o) {
    if (this == o)
      return true;
    return false;
  }
  protected native Object clone();
  public String toString() {
    return ""; //getClass().getName() + '@' + Integer.toHexString(hashCode());
  }
  public final native void notify();
  public final native void notifyAll();
  public final native void wait(long v);
  public final void wait(long a, int b) {
  }
  public final void wait() {
  }
}
