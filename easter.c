/*
 * port of the Multics Easter calculation to C.
 *
 * see the original PL/1 code and especially the
 * informative comments below.
 */

#include <stdio.h>

void easter(int []);

char *mon[] = {
	"", "Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"
};

int
main(int argc, char *argv[])
{
	int date[3]; // year, month, day
	char c;

	if (argc == 1) {
		printf("usage: easter year\n");
		return(0);
	}
	date[2]=0;
	while(c=*argv[1]++)
		date[2]=date[2]*10+c-48;
	easter(date);

	printf("%4d %s %2d\n", date[2], mon[date[1]], date[0]);
}

/*
 * expects year to be passed in ymd[2],
 * month and day will be populated in
 * ymd[1] and ymd[0] respectively.
 */
void
easter(int ymd[3]) 
{
#define MOD(j,n,d) { j=n; while ((j) >= (d)) { (j) -= (d); } }
#define DIV(j,n,d) { int p=n; j=0; while ((p)>=(d)) { j++; (p) -= (d); } }

	int a, b, c, d, e, g, h, i, k, l, m;

	MOD(a,ymd[2],19);
	DIV(b,ymd[2],100);
	MOD(c,ymd[2],100);
	DIV(d,b,4);
	MOD(e,b,4);
	DIV(i,c,4);
	MOD(k,c,4);
	DIV(g,8*b+13,25);
	MOD(h,19*a+b-d-g+15,30);
	MOD(l,2*e+2*i-k+32-h,7);
	DIV(m,a+11*h+19*l,433);
	DIV(ymd[1],h+l-7*m+90,25);
	MOD(ymd[0],h+l-7*m+33*ymd[1]+19,32);
}


/*******************************************************************

The original Multics code and
comments are copied from:
https://multicians.org/calendar.html
The programmer was Dennis Capps, see
https://multicians.org/thvv/gauss.html.


calculate_easter:   proc(year, month, day);

declare
day fixed bin,
month   fixed bin,
year    fixed bin,
(a, b, c, d, e, g, h, i, k, l, m) fixed bin;

     * The following calculation of the Date for Easter follows the algorithm
       given in the New Scientist magazine, issue No. 228 (Vol. 9) page 828 (30 March 1961). *

    a = mod(year,19);	* Find position of year in 19-year Lunar Cycle, called the Golden Number. *
    b = divide(year,100,35);
    c = mod(year,100);	* b is century number, c is year number within century*
    d = divide(b,4,35);
    e = mod(b,4);	* These are used in leap year adjustments. *
    i = divide(c,4,35);
    k = mod(c,4);	* Also related to leap year. *

     * The next step computes a correction factor used in the following step
       which computes the number of days between the spring equinox
       and the first full moon thereafter.  The correction factor is needed
       to keep the approximation in line with the observed behavior of the moon.
       It moves the full moon date back by one day eight times in every 2500 years,
       in century years three apart, with four years at the end of the cycle.
       The constant 13 corrects the correction for the fact that this
       cycle was decreed to start in the year 1800. *
    g = divide(8*b+13,25,35);

     * Now the number of days after the equinox (21 March, by definition) that
       we find the next full moon.  This is a number between 0 and 29.
       The term 19*a advances the full moon 19 days for each year of the
       Lunar Cycle, for a total of 361 days in the 19 years.  The other 4.24 days
       are made up when a returns to zero on the next cycle.  Thus, the
       full moon dates repeat every 19 years.  The term b-d advances the
       date by one day for three out of every four century years, the
       years which are not leap years although divisible by 4.
       The term g is the correction factor calculated above, and 15
       adjusts this whole calculation to the actual conditions at that
       date on which the scheme began, probably in Oct of 1582. *
    h = mod(19*a + b - d - g + 15, 30);

     * Now we are interested in how many days we have to wait after the
       full moon until we get a Sunday (which has to be definitely after
       the full moon).  The following step calculates a number l which is
       one less than the number of days.  Every ordinary year ends on the
       same day of the week on which it started;  a leap year ends on the
       day of the week following the one on which it started.  Thus, if
       it is known on what day of the week a date occurred in any year
       it is possible to calculate its day of the week in another year
       by marching through the week one day for each regular year and
       two for each leap year.
            The term k is the number of ordinary years
       since the last leap year;  each such year brings the date of the
       full moon one day closer to Sunday, and so reduces the number of
       days to be waited (unless it goes negative, but modular arithmetic
       theory makes -1 = 6 where the modulus is 7).
            The term i is the number of leap years so far in the current century.
       each leap year has with it three ordinary years, and each such group
       advances the day of the week by 5 days.  But in modulo 7 arithmetic
       subtracting 5 days is equivalent to adding 2 days.  So we add
       two days for each group of four years in the current century.
            Since a century consists of 25 groups of four years, it advances
       the day of the week by 124 or 125 days depending on whether the
       century year is an ordinary or leap year.  The remainders when
       these numbers are divided by seven are 5 and 6 respectively.
       The term e is the number of ordinary century years since the
       last leap century year.  As with the groups of four years, we
       add two days for each rather than subtract 5 for each.
            Every fourth century year is a leap year;  therefore,
       each group of four centuries advances the day of the week by
       3*5+6 = 21 days, or 0 in modulo 7 arithmetic, and no
       term is necessary for time before the last leap century year.
       The constant term 32 adjusts the calculation for the day of the
       week of the equinox when the scheme was put into effect.  It also
       is larger than necessary by 28 in order to assure that the
       subtractions of k and h never reduce the dividend below 0.
            Thus, mod(2*e + 2*i - k + 32, 7) gives one less than the number
       of days between the equinox and its following Sunday.  But we need to
       calculate the number of days after the full moon.  The term h,
       calculated in the previous step, gives the number of days after
       the equinox that the full moon occurs.  Each of those days brings
       the full moon closer to the actual Sunday of Easter,
       so it reduces the number of days after the full moon until Easter.
       (Again, if h > 6, modular arithmetic theory readjusts the result to
       another cycle of 0 to 6, and here the constant 32 keeps the dividend > 0.)   *
    l = mod(2*e + 2*i - k + 32 - h, 7);

     * The calendar set up by Pope Gregory XIII and his advisor, the astronomer
       Clavius, provided for official full moon dates as well as matching
       the equinoxes and solstices with their nominal dates.  But, since
       the period of the moon is not an exact number of days, some fudging
       was needed here as elsewhere in the calendar system.  Some of the
       periods between successive full moons in the Lunar Cycle are 30 days,
       some 29 days.  Clavius then arranged the periods carefully so
       that if a full moon fell on 20 March (the day before the equinox),
       the period following it would be of 29 days.  The effect of this
       arrangement is that Easter can never occur later than 25 April.
       The above calculations assume uniform 30-day lunar periods.  In rare
       cases (e.g., 1954 and 1981) one of these 29-day lunar periods causes
       the full moon to fall on a Saturday where a 30-day period would put
       it on a Sunday.  The following step calculates the fudge factor for
       this situation.  The result m is 0 if no fudging is necessary, or
       1 if fudging is required.     *
    m = divide(a + 11*h + 19*l, 433, 35);

     * Now we have calculated the number of days which will elapse between
       21 march and Easter: h + (l + 1) - 7*m.  The next two steps
       turn this into a month and day.  In the first expression, the constant
       90 assures that the the quotient will be at least 3 (= March).
       If the elapsed days exceed 9, then the quotient will be 4 (= April).
       In the second expression, if month = 3 then 33*month + 19 = 118 and the
       remainder of that part of the expression is 22;  when month = 3,
       l + h - 7*m < 10, so 22 < day <= 31.
       If month = 4, 33*month = 132, and since h + l - 7*m > 9, the whole
       expression satisfies 5*32 = 160 < expr.  The remainder is greater
       than 0 and less than 26.   *
    month   = divide(h + l - 7*m + 90, 25, 35);
    day = mod(h + l - 7*m +33*month + 19, 32);

    return;

end calculate_easter;

*******************************************************************/
