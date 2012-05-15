package tests;

class Virtual1Apple {
  public int test(int x) {
    return x * x;
  }
}

class Virtual1 extends Virtual1Apple {
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
