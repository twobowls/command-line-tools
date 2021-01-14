/*
 * pretty printer for json
 *
 *
 * examples: stdin, stdout:     ppjson < file.json > ppfile.json
 *           named file:        ppjson file.json
 *
 * when you need to see the value of a particular tag ppjson
 * works well in a pipeline:   cat alloneline.json | ppjson | grep TAG
 *
 * 9/11/2017   first written
 * 7/1/2020    backslash passthrough
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

int n=4;
int i=0;
char nl='\n';
char sp = ' ';

#define SYNTAX  0
#define QUOTED  1

void
indent()
{
	int x;

	if (i < 0)
		i=0;
	x=i*n;
	write(1, &nl, 1);
	while (x--)
		write(1, &sp, 1);
}
 
int
main(int argc, char *argv[])
{
	char a;
	int fd, pt;
	pt=SYNTAX;

	if (argc == 1)
		fd = 0;
	else {
		fd = open(argv[1], O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "can't open %s for reading\n", argv[1]);
			exit(1);
		}
	} 
	while (read(fd, &a, 1) == 1) {
		if (a > 0x7f) {
			fprintf(stderr, "binary\n", argv[1]);
			exit(1);
		}
		if (a == '\\') {
			write(1, &a, 1);
			read(fd, &a, 1);
			write(1, &a, 1);
			continue;
		}
		if (a == '\n' || a == '\r' || a == '\t' || a == ' ')
			if (pt == SYNTAX)
				continue;

		if (a == '{' && pt == SYNTAX) {
			write(1, &a, 1);
			i++;
			indent();
		} else if (a == '\"' && pt == SYNTAX) {
			pt = QUOTED;
			write(1, &a, 1);
		} else if (a == '\"' && pt == QUOTED) {
			pt = SYNTAX;
			write(1, &a, 1);
		} else if (a == '}' && pt == SYNTAX) {
			i--;
			indent();
			write(1, &a, 1);
		} else if (a == ',' && pt == SYNTAX) {
			write(1, &a, 1);
			indent();
		} else if (a == ',' && pt == QUOTED) {
			write(1, &a, 1);
		} else
			write(1, &a, 1);
	}
	write(1, &nl, 1);

	exit(0);
}
