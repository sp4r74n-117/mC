int read_int();
void print_int(int);

int mod(int a, int b)
{
  return a - ((a / b) * b);
}

int gcd (int a, int b)
{
  int c;
  while ( a != 0 ) {
     c = a;
     a = mod(b, a);
     b = c;
  }
  return b;
}

int main()
{
  int a = read_int();
  int b = read_int();
  print_int(gcd(a, b));
  return 0;
}
