package java.lang;

interface Throwable {
}

public class Exception implements Throwable {
  private Exception             cause;
  private String                msg;
  private int                   code;

  public Exception(String msg) {
    this.msg = msg;
    this.cause = null;
  }
  
  public Exception() {
  }
  
  public Exception(String msg, Exception cause) {
    this.msg = msg;
    this.cause = cause;
  }
  
  public Exception getCause() {
    return this.cause;
  }

  public String getMessage() {
    return this.msg;
  }
}

