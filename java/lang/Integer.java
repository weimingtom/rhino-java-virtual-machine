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
      
    // -394
    m = 1;
    v = 0;
    for (x = d.length - 1; x > w; --x) {
      v += (d[x] - '0') * m;
      m = m * 10;
    }
    return v * sign;
  }
}