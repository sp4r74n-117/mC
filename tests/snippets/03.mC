int read_int();
void print_int(int);

int main()
{
  int n = read_int();
  if (n)
  {
    print_int(1);
  }
  else
  {
    print_int(0);
  }
  return 0;
}
