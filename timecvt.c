/*
 * timecvt()
 *
 * Adopted from the book "Advanced Unix Programming" by M. J. Rochkind
 * with a small repair to the leap year calculation.
 *
 * E. Collins  22-Nov-1997
 */

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "timecvt.h"

/* Error return codes */
#define BADLEN 		-1
#define BADNUM 		-2
#define BADMONTH 	-3
#define BADDAY 		-4
#define BADHOUR 	-5
#define BADMINUTE 	-6
#define BADSECOND 	-7


time_t
timecvt (char *atime)
{
	time_t tm;
	int i, n, m, days;
	char s[3], *getenv(), *tz;
	int isleapyear;
	extern struct tm *localtime();

	if (strlen (atime) != 12)
		return (BADLEN);

	for (i=0; i<12; i++)
		if (atime[i] < '0' || atime[i] > '9')
			return (BADNUM);

	s[2] = '\0';
	for (i=0; i<12; i+=2) {
		s[0] = atime[i];
		s[1] = atime[i+1];
		n = atoi (s);
		switch (i) {
		case 0: /* YY: year */

		/*
 		 * 2000 is a leap year. Given an operating range
		 * of 1970 to 2038 the test below is sufficient.
		 */
			isleapyear = (n%4==0);

			if (n<70)
				n+=100; /* years after 2000 */
			days = (n-70) * 365;
			days += ((n-69) / 4); /* previous leap days */
			break;
		case 2: /* MM: month */
			m=n;
			switch (n) {
			case 12:
				days += 30; /* Nov */
			case 11:
				days += 31; /* Oct */
			case 10:
				days += 30; /* Sep */
			case 9:
				days += 31; /* Aug */
			case 8:
				days += 31; /* Jul */
			case 7:
				days += 30; /* Jun */
			case 6:
				days += 31; /* May */
			case 5:
				days += 30; /* Apr */
			case 4:
				days += 31; /* Mar */
			case 3:
				days += (isleapyear?29:28); /* Feb */
			case 2:
				days += 31; /* Jan */
			case 1:
				break;
			default:
				return (BADMONTH);
			}
		case 4: /* DD: day */
			if (n>31||n==0||(m==2&&!isleapyear&&n==29))
				return (BADDAY);
			tm = (days + n-1) * 24 * 60 * 60;
			break;
		case 6: /* hh: hour */
			if (n>23)
				return (BADHOUR);
			tm += n * 60 * 60;
			break;
		case 8: /* mm: minute */
			if (n>59)
				return (BADMINUTE);
			tm += n * 60;
			break;
		case 10: /* ss: second */
			if (n>59)
				return (BADSECOND);
			tm += n;
			break;
		default:
			break;
		}
	}
	if (localtime (&tm)->tm_isdst)
		tm -= 60 * 60; /* adjust for daylight savings time */
	return tm < 0 ? BADRANGE : tm + (60 * 60 * 6);
}


