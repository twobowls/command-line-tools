/*
 * rate
 *
 * efficiently calculate accurate annual percentage rate
 * required to generate endamt from begamt in numyears. 
 *
 * 22 March 2000   E. Collins
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PRECISION (double)100000000000.0
#define ABS(x) ((x)<0?(x)*-1:(x))

int
main(int argc, char *argv[])
{
	double b, e, p, i = .15 ,div = .01, prec;
	int y = 0, cntup = 0, cntdn = 0;

	if (argc != 4) {
		printf("usage: %s begamt endamt numyears\n", argv[0]);
		exit(1);
	}

	b = atof(argv[1]);
	e = atof(argv[2]);
	if (b >= e) {
		printf("ending amount must be greater than beginning amount\n");
		exit(1);
	}

	prec = ABS((e/PRECISION)*((int)e/(int)b)) ;
	
	for (;;) {
		for (p = b, y = atoi(argv[3]); y > 0; y-- )
			p += p * i;

		if (e - p > prec && ++cntup) {
			if (cntdn)
				div /= 2;
			else
				div *= 2;
			i += div;
		} else if (p - e > prec && ++cntdn) {
			if (cntup)
				div /= 2;
			else
				div *= 2;
			i -= div;
		} else
			break;
	}
	printf("%0.9f\n", i * 100);
	fsync(0);
	exit(0);
}
