
/*
 * build: cc -o caltosys caltosys.c timecvt.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "timecvt.h"

struct s_date {
	char str[4];
	char num[3];
} ; 


struct s_date mon[13]={ {"Nul", "00"},
			{"Jan", "01"},
			{"Feb", "02"},
			{"Mar", "03"},
			{"Apr", "04"},
			{"May", "05"},
			{"Jun", "06"},
			{"Jul", "07"},
			{"Aug", "08"},
			{"Sep", "09"},
			{"Oct", "10"},
			{"Nov", "11"},
			{"Dec", "12"} } ;
	

int
main (int argc, char *argv[])
{
	time_t tm;
	int i = 0;

	char arg[13] ;

	char *pday;
	char *pmon;
	char *pyr ;
	char *phr;
	char *pmin;
	char *psec;

	memset (arg, 0, 13);
	if (argc != 6 && argc != 7) {
		printf ("usage: caltosys day mon hh:mm:ss tz year\n");
		printf ("or: caltosys day mon hh:mm:ss year\n");
		printf ("example1: timecvt Tue Sep 20 22:37:24 CDT 2005\n");
		printf ("example2: timecvt Tue Sep 20 22:37:24 2005\n");
		exit (1);
	}

	pmon = argv[2];
	for (i=0; i<sizeof mon; i++) {
		if (! strcmp (pmon, mon[i].str)) {
			pmon=mon[i].num;
			break;
		}
	}
	if (i==sizeof mon) {
		printf ("month parse failed\n");
		exit (1);
	}
	
	pday = argv[3];
	if (argc == 7)
		pyr = (char *)&argv[6][2];
	else
		pyr = (char *)&argv[5][2];
	phr = argv[4];
	pmin = (char *)&argv[4][3];
	psec = (char *)&argv[4][6];
	argv[4][2] = '\0';
	argv[4][5] = '\0';
	
	/* printf ("yy %s  mm %s dd %s hh %s mn %s ss %s\n",
	pyr, pmon, pday, phr, pmin, psec); */

	strcat (arg, pyr);
	strcat (arg, pmon);
	if (strlen(pday) == 1) {
		pday = (char *)malloc(3);
		pday[0] = '0';
		pday[1] = argv[3][0];
		pday[2] = '\0';
	}
	strcat (arg, pday);
	strcat (arg, phr);
	strcat (arg, pmin);
	strcat (arg, psec);
	/* printf ("arg %s\n", arg); */

	tm = timecvt (arg);

        if (tm < 0) {
		printf ("%s\n", tm==BADLEN?"bad length":
					tm==BADNUM?"bad number":
					tm==BADMONTH?"bad month":
					tm==BADDAY?"bad day":
					tm==BADHOUR?"bad hour":
					tm==BADMINUTE?"bad minute":
					tm==BADSECOND?"bad second":
					tm==BADRANGE?"out of range":
					"error");
		exit (1);
	}

	/* printf ("you entered %s%d\n", ctime (&tm), tm); */
	printf ("%ld\n", tm);
	exit (0);
}
