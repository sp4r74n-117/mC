void print_int(int);

int sum(int max)
{
  int sum = 0;
  for (int i = 1; i <= max; i = i + 1)
  {
    sum = sum + i;
  }
  return sum;
}

int main()
{
  int max = 10000;
  for (int i = 0; i < 100; i = i + 1)
  {
    int r = sum(max);
    if (r != 50005000)
    {
      print_int(r);
      return 1;
    }
  }
  return 0;
}
