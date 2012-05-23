class Test {
  public long main() {
    String s;
    s = Integer.toString(12345678);
    Core.Core.PrintString(s);
    return 4;
  }
  public static void main(String[] args) {
    Test         o;
    o = new Test();
    System.out.println(o.main());
  }
}
