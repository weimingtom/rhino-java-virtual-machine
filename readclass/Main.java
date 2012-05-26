import Util.MemoryStream;

class JVMPoolMethodRef {
  int           type;
  int           classIndex;
  int           nameAndTypeIndex;
}

class JVMPoolString {
  int           type;
  int           stringIndex;
}

class JVMPoolClass {
  int           type;
  int           nameIndex;
}

class JVMPoolFieldRef {
  int           type;
  int           classIndex;
  int           nameAndTypeIndex;
}

class JVMPoolUtf8 {
  int           type;
  byte[]        data;
}

class JVMPoolNameAndType {
  int           type;
  int           nameIndex;
  int           descIndex;
}

class Main {
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
    int                 y;
    int                 tag;
    JVMPoolMethodRef    pmr;
    JVMPoolString       pst;
    JVMPoolClass        pcl;
    JVMPoolFieldRef     pfr;
    JVMPoolNameAndType  pnt;
    Object[]            pool;
    JVMPoolUtf8         pu8;

    buf = Core.Core.ReadFile("Test.class");

    ms = new MemoryStream(buf, 0);

    magic = ms.ReadB32();
    minver = ms.ReadB16();
    majver = ms.ReadB16();
    cpoolcnt = ms.ReadB16();

    pool = new Object[cpoolcnt];
    for (x = 0; x < cpoolcnt - 1; ++x) {
        tag = ms.Read8();
        switch (tag) {
          case 12:
            pnt = new JVMPoolNameAndType();
            pnt.nameIndex = ms.ReadB16();
            pnt.descIndex = ms.ReadB16();
            pnt.type = 12;
            break;
          case 1:
            pu8 = new JVMPoolUtf8();
            y = ms.ReadB16();
            pu8.data = ms.Read(y);
            pu8.type = 1;
            break;
          case 10:
            pmr = new JVMPoolMethodRef();
            pmr.classIndex = ms.ReadB16();
            pmr.nameAndTypeIndex = ms.ReadB16();
            pmr.type = 10;
            pool[x] = pmr;
            break;
          case 8:
            pst = new JVMPoolString();
            pst.stringIndex = ms.ReadB16();
            pst.type = 8;
            pool[x] = pst;
            break;
          case 7:
            pcl = new JVMPoolClass();
            pcl.nameIndex = ms.ReadB16();
            pcl.type = 7;
            break;
          case 9:
            pfr = new JVMPoolFieldRef();
            pfr.classIndex = ms.ReadB16();
            pfr.nameAndTypeIndex = ms.ReadB16();
            pfr.type = 9;
            break;
          default:
            Core.Core.PrintString(String.format("unknown tag %i\n", tag));
            Core.Core.Exit(-1);
        }
    }
    
    Core.Core.PrintString(String.format("magic:%i minver:%i majver:%i", magic, minver, majver));

    return 1;
  }

  public static void main(String[] args) {
    Main         o;
    o = new Main();
    System.out.println(o.main());
  }
}
