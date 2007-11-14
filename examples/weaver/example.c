class Generic

{
public:
  void foo(int a)
  
  {  
    bar(a);
  }
  
  virtual void bar(int b)
  
  {
    printf("generic\n");
  }
};
  
class Specific : public Generic

{
public:
  virtual void bar(int b)
  
  {
    printf("%d\n", b);
  }
};

void doSomething(Generic *X)

{
  X->foo(42);
}

void main()

{
  doSomething(new Specific());
}
