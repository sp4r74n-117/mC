void print_int(int);
int read_int();

int main()
{
  int A[5];
  int B[5];
  for (int i = 0; i < 5; i = i + 1)
  {
    A[i] = read_int();
    B[i] = read_int();
  }

  int C[5];
  for (int i = 0; i < 5; i = i + 1)
  {
    C[i] = A[i] * B[i];
  }

  int sum = 0;
  for (int i = 0; i < 5; i = i + 1)
  {
    sum = sum + C[i];
  }
  print_int(sum);
  return 0;
}
