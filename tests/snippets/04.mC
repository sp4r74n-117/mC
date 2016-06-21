int read_int();
void print_int(int);

int main()
{
  int n = 1;
  while (n == 1)
  {
    print_int(42);
    n = read_int();
  }
  return 0;
}
