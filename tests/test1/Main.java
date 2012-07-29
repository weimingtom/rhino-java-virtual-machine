class Main {
  public long main() {
    Character           c;
    byte[]              buf;
    byte[]              ident;

    buf = Core.LoadResource("rhino");
    
    c = new Character((char)65);
    
    return (int)Character.getType((char)65);
  }
}
