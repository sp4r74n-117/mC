void print_float(float);

float pi(float num_its) {
	float res = 0.0;
	float i = 0.0;
	for(; i < num_its; i=i+2.0) {
		res = res + (4.0 /  (1.0 + (i*2.0)));
		res = res - (4.0 /  (1.0 + ((i+1.0)*2.0)));
	}
	return res;
}

int main() {
	float res = pi(2000.0);
	print_float(res);
	return 0;
}
