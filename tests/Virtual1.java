//./tests/Virtual1Apple.class ./tests/Virtual1Peach.class ./java/lang/Object.class :tests/Virtual1
package tests;

class Virtual1Apple {
  public int test(int x) {
    return x * x;
  }
}

class Virtual1Peach extends Virtual1Apple {
  public int test(int x) {
    return 55 * 8;
  }
}

class Virtual1 extends Virtual1Peach {
  public int main() {
    return test(8);
  }

  public static void main(String[] arg) {
    Virtual1        t;

    t = new Virtual1();
    System.out.print(t.main());
    return;
  }
}
