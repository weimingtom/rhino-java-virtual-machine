public class OutOfMemoryError extends Exception {
  public OutOfMemoryError() { }
  public OutOfMemoryError(String msg) {
    this.msg = msg;
  }
}