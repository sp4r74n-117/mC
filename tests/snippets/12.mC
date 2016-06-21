int read_int();
void print_int(int);

int main()
{
  int n = read_int();
  for (int i = 1; i < n; i = i + 1)
  {
    int a[i];
    for (int j = 0; j < i; j = j + 1)
    {
      a[j] = j;
    }

    print_int(0);

    int sum = 0;
    for (int j = 0; j < i; j = j + 1)
    {
      sum = sum + a[j];
    }
    print_int(sum);
  }
  return 0;
}
