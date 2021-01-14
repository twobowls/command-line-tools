/*
 * Calculate p&i payments, showing monthly schedule.
 * 
 * E. Collins   January 4, 2004
 */
#include <stdlib.h>
#include <stdio.h>

double pwr(double i, int m);
double pmt(double p, double i, int m);
double interest (double p, double i);

int main(int argc, char *argv[]) {
	int m, n=1, y=1, bpa, bpm;
	double p, i, cb, aba, ba, mp, ci, ototi, totp = 0.0, toti = 0.0;

	if (argc != 6 && argc != 4) {
		printf ("Usage: %s principle rate years\n", argv[0]);
		printf ("or:    %s principle rate years bigpaymentamt bigpaymentmonth\n", argv[0]);
		exit(0);
	}

	p = atof(argv[1]);
	i = atof(argv[2])/1200;
	m = atoi(argv[3])*12;

	if (argc == 6) {
		bpa = atoi(argv[4]);
		bpm = atoi(argv[5]);
	} else {
		bpa = 0;
		bpm = 0;
	}

	mp = pmt(p,i,m);

	printf ("\n\n            --- %.2f at %.2f percent for %d ",
			p, i * 1200, m/12);
	if (m/12 > 1.0)
		printf ("years ---\n\n");
	else
		printf ("year ---\n\n");


	if (argc == 6) {
		cb = p;
		while (n<=m) {
			ci = interest(cb, i);
			toti += ci;
			totp += (mp - ci);
			ba = cb - (mp - ci);
			if (ba<0.0)
				ba = 0.0;
			// printf ("%3d  %4d   %9.2f   %9.2f   %9.2f   %9.2f   %9.2f\n", 
			// 		y, n, mp - ci, totp, ci, toti, ba);
			cb -= (mp - ci);
			if (n%12 == 0) {
			//	printf ("\n");
				y++;
			}
			n++;
		}
	}
	ototi = toti;

	n=1, y=1;
	totp=0.0, toti=0.0, aba = 0.0;

	printf (" yr   pmt   principle     cml pri    interest     cml int     balance\n");
	printf ("=====================================================================\n\n");

	cb = p;
	while (n<=m) {
		ci = interest(cb, i);
		toti += ci;
		totp += (mp - ci);
		ba = cb - (mp - ci);
		if (ba<0.0) {
			aba=abs(ba);
		 	ba = 0.0;
			totp -= aba;
		}
		p=(aba>0.0?mp-ci-aba:mp-ci);
		if (n == bpm)
			p += bpa - mp;
		printf ("%3d  %4d   %9.2f   %9.2f   %9.2f   %9.2f   %9.2f\n", 
				y, n, p, totp, ci, toti, ba);
		cb -= (mp - ci);
		if (n%12 == 0) {
			printf ("\n");
			y++;
		}
		n++;
		if (n == bpm) {
			cb -= bpa;
			totp += bpa;
		}
		if (cb <= 0)
			break;
	}
	printf ("\n%.2f  monthly payment", mp);
	printf ("   %.2f in total payments\n\n", mp * n + bpa - aba);
	printf ("total principal: %.2f   total interest: %.2f\n\n", totp, toti);
	if (argc == 6)
		printf ("   %.2f interest saved on early payment of %d\n\n", ototi-toti, bpa);

}

double pwr(double i, int m) {
	int y;
	double x=i;
	for (y=0; y<m; y++)
		x=(i*x);
	return x;
}

double interest (double p, double i) {
	return p*(i+1) - p;
}

double pmt(double p, double i, int m) {
	double r = pwr(i+1, m-1);
	return p*r*i/(r-1);
}
