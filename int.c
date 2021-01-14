#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	double f, p;
	int i;

	if (argc != 4) {
		printf ("usage: %s amount percent numyears\n", argv[0]);
		exit (0);
	}

	f = atof(argv[1]);
	p = atof(argv[2]);
	i = atoi(argv[3]);

	while (i-- > 0) {
		f += f * (p / 100);
		printf ("%0.2f\n", f);
	}
}
