{
	int n = 42;
	int c;

	int is_prime = 0;

	if ( n == 2 ) {
		is_prime = 1;
	} else {
		for ( c = 2; c <= n - 1 ; c = c + 1) {
			if ( (n - ((n / c) * c)) == 0 ) {
				c = n;
			}
		}
	}
	if ( c != n ) {
		is_prime = 0;
	} else {
		is_prime = 1;
	}
}
