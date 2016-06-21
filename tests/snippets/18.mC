int read_int();
float read_float();
void print_int(int);
void print_float(float);

int main()
{
  int m;
  int n;
  int c;
  int d;
  int k;
  int p;
  int q;
  float sum = 0.0f;

  m = read_int();
  n = read_int();

  float first[m][n];
  for (c = 0; c < m; c = c + 1)
  {
    for (d = 0; d < n; d = d + 1)
    {
      first[c][d] = read_float();
    }
  }

  p = read_int();
  q = read_int();

  if (n != p)
  {
    print_int(0);
    return 1;
  }

  float second[p][q];
  float multiply[m][q];

  for (c = 0; c < p; c = c + 1)
  {
    for (d = 0; d < q; d = d + 1)
    {
      second[c][d] = read_float();
    }
  }

  for (c = 0; c < m; c = c + 1)
  {
    for (d = 0; d < q; d = d + 1)
    {
      for (k = 0; k < p; k = k + 1)
      {
        sum = sum + first[c][k]*second[k][d];
      }

      multiply[c][d] = sum;
      sum = 0.0f;
    }
  }

  for (c = 0; c < m; c = c + 1)
  {
    for (d = 0; d < q; d = d + 1)
    {
      print_float(multiply[c][d]);
    }
  }
  return 0;
}
