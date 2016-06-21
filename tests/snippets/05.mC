void print_int(int);

int main()
{
  int n = 0;
  while (n < 7)
  {
    print_int(n);
    n = n + 1;
  }
  n = 7;
  print_int(42);
  while (n >= 0)
  {
    print_int(n);
    n = n - 1;
  }
  print_int(n);
  return 0;
}
