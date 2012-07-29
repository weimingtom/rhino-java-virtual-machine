package java.lang;

public class Integer extends Number implements Comparable {
  protected int                   v;
  public Integer() { 
  }
  public Integer(int i) {
    this.v = i;
  }
  
  static Integer decode(String nm) {
    return new Integer(0);
  }
  
  static Integer getInteger(String nm) {
    return null;
  }
  
  static Integer getInteger(String nm, int val) {
    return null;
  }
  
  static Integer getInteger(String num, Integer val) {
    return null;
  }
  
  public byte byteValue() {
    return (byte)v;
  }
  
  public double doubleValue() {
    return (double)v;
  }
  
  public float floatValue() {
    return (float)v;
  }
  
  public int intValue() {
    return (int)v;
  }
  
  public long longValue() {
    return (long)v;
  }
  
  public short shortValue() {
    return (short)v;
  }
  
  public static Integer valueOf(int i) {
    Integer     ni;
    ni = new Integer(i);
    return ni;
  }

  public String toString() {
    return Integer.toString(v);
  }
  
  public int compareTo(Integer anotherInteger) {
    if (v == anotherInteger.v)
      return 0;
    if (v < anotherInteger.v)
      return -1;
    return 1;
  }
  
  public int compareTo(Object o) throws ClassCastException {
    if (!(o instanceof Integer))
      throw new ClassCastException();
    return compareTo((Integer)o);
  }
  
  public int hashCode() {
    return v;
  }

  public static String toString(int i) {
    byte[]      b;
    byte[]      _b;
    int         m;
    int         x;
    int         c;
    String      s;
    int         zc;
    int         gotr;

    b = new byte[32];
    m = 1000000000;
    x = 0;
    zc = 0;
    gotr = 0;
    if ((i & 0x80000000) == 0x80000000) {
      b[x] = '-';
      ++x;
      // convert to positive number
      i = ~i + 1;
    }
    if (i == 0) {
      return "0";
    }
    while (m > 0) {
      c = i / m;
      i = i - (c * m);
      m = m / 10;
      if (c > 0)
        gotr = 1;
      if (gotr == 1) {
        b[x] = (byte)((int)'0' + c);
        ++x;
      }
    }
    // cleanup original buffer by
    // making a new one of the
    // exact size of the string
    _b = new byte[x];
    for (i = 0; i < x; ++i)
      _b[i] = b[i];
    return new String(_b);
  }
  
  public static int parseInt(String s, int radix) {
    return 0;
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
