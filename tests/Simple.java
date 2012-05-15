// ./java/lang/Object.class ./tests/Simple.class :tests/Simple
package tests;

class Simple {
  public int main() {
    return 1;
  }

  public static void main(String[] arg) {
    Simple        t;
    t = new Simple();
    System.out.print(t.main());
    return;
  }
}
