package java.lang;

public class String {
  private byte[]        data;

  public String() {
  }

  public String(byte[] data) {
    this.data = data;
  }

  public int length() {
    return data.length;
  }

  public native int f(int x);

  public char charAt(int index) {
    return (char)data[index];
  }

  public byte[] getBytes() {
    return data;
  }

  public static int test(int a, int b) {
    return b;
  }

  public static String format(String sfmt, Object... args) {
    byte[]      fmt;
    byte[]      buf;
    byte[]      src;
    int         x;
    int         y;
    int         z;
    int         a;

    fmt = sfmt.data;
    buf = new byte[256];

    a = 0;
    y = 0;
    for (x = 0; x < fmt.length; ++x) {
      if (fmt[x] == '%') {
        switch (fmt[x + 1]) {
          case 's':
            src = ((String)args[a++]).data;
            for (z = 0; z < src.length; ++z)
              buf[y++] = src[z];
            break;
          case 'i':
            buf[y++] = '@';
            break;
          default:
            buf[y++] = fmt[x];
            break;
        }
        x++;
      } else {
        buf[y++] = fmt[x];
      }
    }
    return new String(buf);
  }

  public int compareTo(String str) {
    int         x;
    int         l;
    byte[]      a;
    byte[]      b;

    a = str.data;
    b = this.data;

    if (a.length < b.length)
      return 1;
    if (a.length > b.length)
      return -1;

    for (x = 0; x < a.length; ++x) {
      if (a[x] < b[x])
        return 1;
      if (a[x] > b[x])
        return -1;
    }

    return 0;
  }

  public String concat(String str) {
    byte[]      n;
    int         x;
    int         al;
    int         bl;

    al = data.length;
    bl = str.length();

    n = new byte[al + bl];

    for (x = 0; x < al; ++x) {
      n[x] = data[x];
    }
    for (x = 0; x < bl; ++x) {
      n[x+al] = str.data[x];
    }

    return new String(n);
  }
}