class Peach {
  public Peach() {
  }

  public int memx() {
    return 83 * 3;
  }

  public int holo() {
    int[]       x;
    x = new int[25];
    x[3] = 2;
    x[4] = 2;
    x[0] = x[3] + x[4];

    if (x[0] > 4)
      x[0]++;

    return x[0] + x.length;

    //return x[0] + x.length;
  }
}

class Apple extends Peach {
  public Apple() {
  }

  public int memo() {
    return 10;
  }
}

class Test extends Apple {
  public Test() {
  }

  public int meme() {
    return 11;
  }
  public int main() {
    return holo();
  }
}
