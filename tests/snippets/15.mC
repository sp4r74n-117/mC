void print_int(int);
int read_int();

int main()
{
  int n = read_int();
  int A[n];
  int B[n];
  for (int i = 0; i < n; i = i + 1)
  {
    A[i] = read_int();
    B[i] = read_int();
  }

  int C[n];
  for (int i = 0; i < n; i = i + 1)
  {
    C[i] = A[i] * B[i];
  }

  int sum = 0;
  for (int i = 0; i < n; i = i + 1)
  {
    sum = sum + C[i];
  }
  print_int(sum);
  return 0;
}
