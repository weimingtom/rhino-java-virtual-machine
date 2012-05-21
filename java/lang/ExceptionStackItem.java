package java.lang;

public class ExceptionStackItem {
  ExceptionStackItem            next;
  int                           opcodeIndex;
  String                        methodType;
  String                        methodName;
  String                        className;
}