void print_int(int);
int read_int();

int main()
{
  int m;
  int n;
  int c;
  int d;
  int k;
  int p;
  int q;
  int sum = 0;

  m = read_int();
  n = read_int();

  int first[m][n];
  for (c = 0; c < m; c = c + 1)
  {
    for (d = 0; d < n; d = d + 1)
    {
      first[c][d] = read_int();
    }
  }

  p = read_int();
  q = read_int();

  if (n != p)
  {
    print_int(0);
    return 1;
  }

  int second[p][q];
  int multiply[m][q];

  for (c = 0; c < p; c = c + 1)
  {
    for (d = 0; d < q; d = d + 1)
    {
      second[c][d] = read_int();
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
      sum = 0;
    }
  }

  for (c = 0; c < m; c = c + 1)
  {
    for (d = 0; d < q; d = d + 1)
    {
      print_int(multiply[c][d]);
    }
  }
  return 0;
}
