void start_measurement();
void end_measurement();
int read_int();
void print_int(int);

int fib(int n) {
	if(n<2) return 1;
	return fib(n-1) + fib(n-2);
}	

int main() {	
	start_measurement();
	int res = fib(15);
	end_measurement();
	print_int(res);
	return 0;
}
	
