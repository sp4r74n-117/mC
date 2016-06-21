int read_int();
void print_int(int);

int main()
{
  int n = read_int();
  if (n <= 0)
  {
    return 1;
  }
  int sum = 0;
  for (int i = 1; i <= n; i = i + 1)
  {
    sum = sum + i;
  }
  print_int(sum);
  return 0;
}
