void print_float(float);

float dot_product(int num_elems) {
	float a[num_elems];
	float b[num_elems];
	float init = 2.0;
	for(int i = 0; i<num_elems; i=i+1) {
		a[i] = init;
		b[i] = 2.0;
		init = init + 1.0;
	}
	float res = 0.0;
	for(int i = 0; i<num_elems; i=i+1) {
		res = res + (a[i]*b[i]);
	}
	return res;
}

int main() {
	float res = dot_product(1000);
	print_float(res);
	return 0;
}
