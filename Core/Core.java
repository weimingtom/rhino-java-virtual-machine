/*
  Operating System Services
*/
package Core;

public class Core {
  public native static String[] EnumClasses();
  public native static void Print(String a);
  public native static void Exit(int code);
  public native static Thread ThreadCreate(Object arg);
  public native static byte[] ReadFile(String path);
  public native static void Collect();
} 