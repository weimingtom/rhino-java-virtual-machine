package java.lang;

class Core {
  // read whole file into memory
  public static native byte[] ReadWholeFile(String path);
}