void print_float(float);
float read_float();

int main()
{
  float a1 = read_float();
  float a2 = read_float();
  float b1 = read_float();
  float b2 = read_float();
  print_float((a1*b1) + (a2*b2));
  return 0;
}
