package java.lang;

public class String {
  private char[]        data;

  public String() {
  }

  public String(byte[] _data) {
    int     x;
    int     cnt;
    
    // if they give us half a character just ignore it
    // --kmcguire (not sure if this is correct)
    cnt = _data.length >> 1;
    
    data = new char[cnt];
    
    for (x = 0; x < cnt; ++x) {
      data[x] = (char)((_data[x*2+0] << 8) | _data[x*2+1]);
    }
    
    this.data = data;
  }
  
  public String(char c) {
    data = new char[1];
    data[0] = c;
  }
  
  public String(char[] s) {
    int       x;
    
    data = new char[s.length];
    for (x = 0; x < s.length; ++x) {
      data[x] = s[x];
    }
  }

  public int length() {
    return data.length;
  }

  public native int f(int x);

  public char charAt(int index) {
    return (char)data[index];
  }

  public byte[] getBytes() {
    byte[]    _data;
    int       x;
    
    _data = new byte[data.length * 2];
    
    for (x = 0; x < data.length; ++x) {
      _data[x * 2 + 0] = (byte)((data[x] >> 8) & 0xff);
      _data[x * 2 + 1] = (byte)(data[x] & 0xff);
    }
    
    return _data;
  }
  
  public char[] toCharArray() {
    char[]    _data;
    int       x;
    
    _data = new char[data.length];
    for (x = 0; x < data.length; ++x)
      _data[x] = data[x];
    return _data;
  }

  public static String format(String sfmt, Object... args) {
    char[]      fmt;
    char[]      buf;
    char[]      _buf;
    char[]      src;
    int         x;
    int         y;
    int         z;
    int         a;
    String      _s;
    

    fmt = sfmt.data;
    // i know better can be done here, and should be done, but
    // technically it should at least error out
    // --kmcguire
    buf = new char[256];

    a = 0;
    y = 0;
    for (x = 0; x < fmt.length; ++x) {
      if (fmt[x] == '%') {
        switch (fmt[x + 1]) {
          case 's':
            src = ((String)args[a++]).toCharArray();
            for (z = 0; z < src.length; ++z)
              buf[y++] = src[z];
            break;
          case 'i':
            src = ((Integer)args[a++]).toString().toCharArray();
            for (z = 0; z < src.length; ++z)
              buf[y++] = src[z];
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
    
    // create new buf of the exact needed size
    _buf = new char[y];
    for (x = 0; x < y; ++x)
      _buf[x] = buf[x];
    
    return new String(_buf);
  }

  public int compareTo(String str) {
    int         x;
    int         l;
    char[]      a;
    char[]      b;

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
    char[]      n;
    int         x;
    int         al;
    int         bl;

    al = data.length;
    bl = str.length();

    n = new char[al + bl];

    for (x = 0; x < al; ++x) {
      n[x] = data[x];
    }
    for (x = 0; x < bl; ++x) {
      n[x+al] = str.data[x];
    }

    return new String(n);
  }
}
