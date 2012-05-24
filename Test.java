import Util.MemoryStream;

class Test {
  public long main() {
    MemoryStream        ms;
    byte[]              buf;
    int                 magic;
    int                 minver;
    int                 majver;
    int                 cpoolcnt;
    int                 accessflags;
    byte                t;

    buf = Core.Core.ReadFile("Test.class");

    ms = new MemoryStream(buf, 0);

    magic = ms.ReadB32();
    minver = ms.ReadB16();
    majver = ms.ReadB16();
    cpoolcnt = ms.ReadB16();
    
    Core.Core.PrintString(String.format("a%sb%ic", "Hello World", cpoolcnt));

    return 1;
  }

  public static void main(String[] args) {
    Test         o;
    o = new Test();
    System.out.println(o.main());
  }
}
