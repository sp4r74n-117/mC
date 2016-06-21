void print_int(int out);

int sort(int num_elems) {
	int a[num_elems];
	for(int i = 0; i<num_elems; i=i+1) {
		a[i] = num_elems-(i+21+10+1+1);
	}
	for(int i = 0; i<num_elems-1; i=i+1) {
		for(int j = i+1; j<num_elems; j=j+1) {
			if(a[i] > a[j]) {
				int tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;
			}
		}
	}
	return a[0];
}

int main() {
	int res = sort(200);
	print_int(res);
	return 0;
}
