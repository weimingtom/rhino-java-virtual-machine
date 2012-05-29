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
    int                 x;

    buf = Core.Core.ReadFile("Test.class");

    ms = new MemoryStream(buf, 0);

    magic = ms.ReadB32();
    minver = ms.ReadB16();
    majver = ms.ReadB16();
    cpoolcnt = ms.ReadB16()

    for (x = 0; x < cpoolcnt; ++x) {
      
    }
    
    Core.Core.PrintString(String.format("magic:%i minver:%i majver:%i", magic, minver, majver));

    return 1;
  }

  public static void main(String[] args) {
    Test         o;
    o = new Test();
    System.out.println(o.main());
  }
}
