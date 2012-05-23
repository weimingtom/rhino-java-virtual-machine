package java.lang;

public class Integer {
  public int                   v;
  public Integer() { }
  public Integer(int i) {
    this.v = i;
  }
  public static Integer valueOf(int i) {
    Integer     ni;
    ni = new Integer(i);
    return ni;
  }

  public static String toString(int i) {
    byte[]      b;
    int         m;
    int         x;
    int         c;
    String      s;
    int         zc;

    b = new byte[32];
    m = 1000000000;
    x = 0;
    zc = 0;
    while (i > 0) {
      c = i / m;
      i = i - (c * m);
      m = m / 10;
      b[x] = (byte)((int)'0' + c);
      ++x;
    }
    
    return new String(b);
  }

  public static int parseInt(String s) {
    byte[]      d;
    int         sign;
    int         x;
    int         l;
    int         m;
    int         v;
    int         w;

    d = s.getBytes();

    if (d[0] == '-') {
      sign = -1;
      w = 0;
    } else {
      sign = 1;
      w = -1;
    }

    m = 1;
    v = 0;
    for (x = d.length - 1; x > w; --x) {
      v += (d[x] - '0') * m;
      m = m * 10;
    }
    return v * sign;
  }
}