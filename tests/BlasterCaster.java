//./java/lang/Object.class :tests/BlasterCaster
package tests;

class BlasterCaster {
  public BlasterCaster                  aa;
  public BlasterCaster                  bb;

  public int main() {
    short       a;
    short       b;
    short       c;
    int         r;

    aa = new BlasterCaster();
    bb = new BlasterCaster();
    aa.aa = bb;
    bb.aa = aa;

    // test for wrap-around and other stuff
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
    // see if idiv, imul, iadd, and isub are handled correctly
    a = 4;
    b = 5;
    c = 6;
    r = r + a - b + c;

    r = r / 2 * 4;

    r = r % 10;

    r = r << 2;

    r = ~r;

    return r;
  }

  public static void main(String[] arg) {
    BlasterCaster        t;

    t = new BlasterCaster();
    System.out.print(t.main());
    return;
  }
}