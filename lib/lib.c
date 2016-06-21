#include <stdio.h>
#include <inttypes.h>

int read_int();
float read_float();
void print_int(int out);
void print_float(float out);

int read_int() {
	int ret = 0;
	printf("Enter an integer: ");
	scanf("%d", &ret);
	return ret;
}

float read_float() {
	float ret = 0.0f;
	printf("Enter a floating point number: ");
	scanf("%f", &ret);
	return ret;
}

void print_int(int out) {
	printf("%8d\n", out);
}

void print_float(float out) {
	printf("%f\n", out);
}

uint64_t measure() {
	volatile uint32_t a, d;
	__asm__ __volatile__("rdtsc" : "=a"(a), "=d"(d));
	return ((uint64_t)a | ((uint64_t)d << 32));
}

uint64_t g_ts;
void start_measurement() {
	g_ts = measure();
}

void end_measurement() {
	uint64_t t = measure() - g_ts;
	printf("       --|--|--|--\n");
	printf("Time: %" PRId64 " ticks\n", t);
}
