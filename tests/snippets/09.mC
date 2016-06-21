void print_float(float);
float read_float();
int read_int();

int main()
{
  int n = 0;
  while (!n)
  {
    float f = read_float();
    print_float((-f) / 2.0);

    int q = read_int();
    if (!q)
    {
      n = -2;
    }
  }
  return 0;
}
