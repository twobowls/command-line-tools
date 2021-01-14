/*
 * skip.c
 *
 * usage: skip records [limit]
 *
 * read stdin, skip number of lines of text specified by records, then
 * print remainder to stdout. print error message to stderr. If limit is
 * supplied, then print limit records after skipping.
 *
 * example:
 *   skip 100 10 < textfile > outfile 2> skiperror
 *   or
 *   cat textfile | skip 100 10 > outfile 2> skiperror
 *
 * this causes skip to place 10 lines of text data in textfile into outfile
 * beginning with the 101st line. the redirection of stderr to a file 
 * is optional.
 *
 * return value:
 *    skip returns 0 to the shell when there are no errors, non-zero if there
 *    are.
 *
 * errors:
 *    errors cause a message to be printed to stdout and a non-zero exit code.
 *
 * limits:
 *    lines longer than 20479 bytes will cause skip to abort with an error.
 *
 *
 * E. Collins   12-May-1999
 */

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* These values will work under VMS */
#define GOOD 0x08000000
#define BAD  0x08000001

/* handle 20k byte lines */
#define MEMSIZ BUFSIZ*40

int
main(int argc, char *argv[])
{
	char buf[MEMSIZ];
	int rownum, n, m, p, q = 0;

	if (argc != 2 && argc != 3) {
		printf ("Usage: %s records [limit]\n", argv[0]);
		printf ("%s reads stdin and skips the defined\n", argv[0]);
		printf ("number of records. Records read in after that\n");
		printf ("are printed to stdout, up to limit if defined,\n");
		printf ("or the rest of the file if not.\n");
		exit (BAD);
	}

	n = 0, m = 1;
	while (m < argc) {
		while (argv[m][n]) {
			if (! isdigit (argv[m][n++])) {
				fprintf (stderr, "%s",
					"argument must be numeric\n");
				exit (BAD);
			}
		}
		m++, n=0;
	}
	n = (int)atoi (argv[1]);
	if (argc == 3)
		q = (int)atoi (argv[2]);
		
	buf[MEMSIZ - 1] = '@';		/* sentinel */
	rownum = p = 0;
	while (fgets (buf, MEMSIZ, stdin)) {
		if (buf[MEMSIZ - 1] != '@') {
			fprintf (stderr, "%s",
				"data record too long (over 20k)\n");
			exit (BAD);
		}
		rownum++;
		if (rownum > n) {
			if (argc == 3 && p == q)
				break;
			p++;
			printf ("%s", buf);
		}
  	}
	exit (GOOD);
}

