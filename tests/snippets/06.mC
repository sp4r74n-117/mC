int read_int();
void print_int(int);

int fib(int n)
{
  if (n == 0)
  {
    return n;
  }
  if (n == 1)
  {
    return n;
  }

  return fib(n - 1) + fib(n - 2);
}

int main()
{
  print_int(fib(read_int()) * 1);
  return 0;
}
