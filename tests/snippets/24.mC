float pi_even(float, float);
void print_float(float);

float pi_odd(float i, float num_its) {
	if(i >= num_its) {
		return 0.0f;
	} else {
		return ((-4.0f) /  (1.0f + (i*2.0f))) + pi_even(i+1.0f, num_its);
	}
}
float pi_even(float i, float num_its) {
	if(i >= num_its) {
		return 0.0f;
	} else {
		return (4.0f /  (1.0f + (i*2.0f))) + pi_odd(i+1.0f, num_its);
	}
}

float pi(float num_its) {
	return pi_even(0.0f, num_its);
}

int main() {
	float res = pi(2000.0f);
	print_float(res);
	return 0;
}
