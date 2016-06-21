void print_int(int);
void print_float(float);

void wrapped_print_int(int n)
{
	print_int(n);
}

void wrapped_print_float(float n)
{
	print_float(n);
}

void check_float(float b)
{
	if (b < 10.0f)
	{
		print_int(1);
	}
	if (b <= 10.0f)
	{
		print_int(2);
	}
	if (b > 6.6f)
	{
		print_int(3);
	}
	if (b >= 6.6f)
	{
		print_int(4);
	}
	if (b == 7.7f)
	{
		print_int(5);
	}
	if (b != 0.0f)
	{
		print_int(6);
	}

	print_float(b + 10.0f);
	print_float(10.0f + b);
	print_float(b / 3.0f);
	print_float(3.0f / b);
	print_float(b * 7.0f);
	print_float(7.0f * b);
	print_float(b - 5.0f);
	print_float(5.0f - b);
}

void check_int(int a)
{
	if (a < 10)
	{
		print_int(7);
	}
	if (a <= 10)
	{
		print_int(8);
	}
	if (a > 6)
	{
		print_int(9);
	}
	if (a >= 6)
	{
		print_int(10);
	}
	if (a == 7)
	{
		print_int(11);
	}
	if (a != 0)
	{
		print_int(12);
	}

	print_int(a + 10);
	print_int(10 + a);
	print_int(a / 3);
	print_int(3 / a);
	print_int(a * 7);
	print_int(7 * a);
	print_int(a - 5);
	print_int(5 - a);
}

int main()
{
	int a = 7;
	float b = 7.7f;
	wrapped_print_int(a);
	wrapped_print_float(b);
	check_float(b);
	check_int(a);
	return 0;
}
