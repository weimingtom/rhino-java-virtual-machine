class Test {
  public int    ww;
  public int c(int x) throws Exception {
    Exception           e;
    e = new Exception("hello exception world");
    throw e;
  }

  public int b(int x) throws Exception {
    return c(x);
  }

  public int a(int x) throws Exception {
    return b(x);
  }

  public int main() throws Exception {
    //String[]            sa;
    String                s;
    //sa = Core.Core.EnumClasses();
    //s = sa[0].concat(sa[1]);
    //Core.Core.PrintString(String.format("a%sb%ic", "hello world", 5));
    //return toot("hello world", sa[0], sa[1]);
    return a(5);
  }
}
