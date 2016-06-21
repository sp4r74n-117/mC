int read_int();
void print_int(int);

int fib(int n) {
	if(n<2) return 1;
	return fib(n-1) + fib(n-2);
}

int main() {
	int res = fib(15);
	print_int(res);
	return 0;
}
