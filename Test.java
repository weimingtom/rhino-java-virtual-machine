class Test {
  public int c(int x) throws Exception {
    //throw new Exception();
    return 66;
  }

  public int b(int x) throws Exception {
    return c(x);
  }

  public int a(int x) throws Exception {
    try {
      return b(x);
    } catch (Exception e) {
      return 55;
    }
  }

  public int main() throws Exception {
    //String[]            sa;
    //String              s;
    //sa = Core.Core.EnumClasses();
    //s = sa[0].concat(sa[1]);
    //Core.Core.PrintString(String.format("a%sb%%c", "hello world", 5));

    //return toot("hello world", sa[0], sa[1]);
    return a(5);
  }
}
