int read_int();
void print_int(int);

int main()
{
  int i;
  int X[205];
  for(i=1; i<=100; i = i + 1)
  {
      X[2] = X[2*i] + 50;
      X[3] = X[-2*i] + 50;
      X[(2*i)+3] = X[2*i] + 50;
      X[(2*i)+4] = X[2*i] + 50;
  }
  return 0;
}
