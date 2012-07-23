/*
  Operating System Services
*/

public class Core {
  public native static String[] EnumClasses();
  public native static void Print(String a);
  public native static void Exit(int code);
  public native static Thread ThreadCreate(Object arg);
  public native static byte[] LoadResource(String path);
} 
