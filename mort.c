/*
 * Calculate p&i payments.
 * 
 * E. Collins   August 2, 2003
 */
#include <stdlib.h>
#include <stdio.h>

double
pwr(double i, int m) {
	int y;
	double x=i;
	for (y=0; y<m; y++)
		x=i*x;
	return x;
}

double
pmt(double p, double i, int m) {
	double r = pwr(i + 1, m - 1);
	return p*r*i/(r-1);
}

int
main(int argc, char *argv[]) {
	if (argc != 4) {
		printf ("Usage: %s principle rate years\n", argv[0]);
		exit(0);
	}
	printf ("%.2f", pmt(strtod(argv[1],NULL), strtod(argv[2],NULL)/1200,
				atoi(argv[3])*12));
	printf ("   %.2f in total payments\n", pmt(strtod(argv[1],NULL),
				strtod(argv[2],NULL)/1200, atoi(argv[3])*12) *
				atoi(argv[3]) * 12);
}
