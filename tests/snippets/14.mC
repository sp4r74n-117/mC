float read_float();
void print_float(float);
int read_int();

int main()
{
  float A[5];
  float B[5];
  for (int i = 0; i < 5; i = i + 1)
  {
    A[i] = read_float();
    B[i] = read_float();
  }

  float C[5];
  for (int i = 0; i < 5; i = i + 1)
  {
    C[i] = A[i] * B[i];
  }

  float sum = 0.0f;
  for (int i = 0; i < 5; i = i + 1)
  {
    sum = sum + C[i];
  }
  print_float(sum);
  return 0;
}
