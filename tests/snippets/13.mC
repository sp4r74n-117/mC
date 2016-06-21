float read_float();
void print_float(float);
int read_int();

int main()
{
  int n = read_int();
  float A[n];
  float B[n];
  for (int i = 0; i < n; i = i + 1)
  {
    A[i] = read_float();
    B[i] = read_float();
  }

  float C[n];
  for (int i = 0; i < n; i = i + 1)
  {
    C[i] = A[i] * B[0];
  }

  float sum = 0.0f;
  for (int i = 0; i < n; i = i + 1)
  {
    sum = sum + C[i];
  }
  print_float(sum);
  return 0;
}
