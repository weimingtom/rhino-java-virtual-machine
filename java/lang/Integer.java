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
}