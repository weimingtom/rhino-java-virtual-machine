//./tests/BlasterCaster.class ./java/lang/Object.class :tests/BlasterCaster
package tests;

class BlasterCaster {
  public int main() {
    short       a;
    short       b;
    short       c;
    int         r;

    // test for wrap-around
    r = 0;
    a = (short)0xffff;
    b = 2;
    c = (short)(a + b);
    if (c == 1)
      r++;

    a = (short)0x7fff;
    b = (short)0x0002;
    c = (short)(a + b);
    if (c != 0)
      r = c;

    a = (short)r;
    r = r + a;

    return r;
  }

  public static void main(String[] arg) {
    BlasterCaster        t;

    t = new BlasterCaster();
    System.out.print(t.main());
    return;
  }
}