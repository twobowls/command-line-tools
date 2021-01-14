/*
 * pretty printer for xml
 *
 * format xml one tag per line. This makes it easy to find fields
 * or extract them with grep.
 *
 * examples: stdin, stdout:     ppxml < file.xml > ppfile.xml
 *           named file         ppxml file.xml
 *
 * when you need to see the value of a particular tag ppxml
 * works well in a pipeline:   cat alloneline.xml | ppxml | grep TAG
 *
 * bugs:
 * ppxml doesn't care about cdata and might corrupt it.
 * data containing a newline will have adjacent whitespace trimmed.
 *
 * 9/18/2015   first written
 * 1/11/2016   accept input from stdin or filename argument. configurable
 *             indent went away.
 * 1/4/2017    empty open/close tags on the same line,
 *             don't filter spaces from tag payload 
 * 1/6/2017    accept pre-pretty printed xml without barfing
 * 2/3/2017    fix bug in indenting where tag payload is a self-closed tag
 * 7/11/2017   don't indent for comments
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

int n=4;
int i=-1;
char nl='\n';
char sp=' ';

#define OPEN    1
#define CLOSE   2
#define COMMENT 4
#define VERSION 8

void
indent()
{
	static int ft=1;
	int x;
	char t;

	if (i < 0)
		i=0;
	x=i*n;
	if (!ft)
		write(1, &nl, 1);
	if (ft)
		ft=0;
	while (x--)
		write(1, &sp, 1);
	
}
 
int
main(int argc, char *argv[])
{
	char a, b, c, d;
	int s, t, fd, pt;
	i=-1; s=0; t=0; pt=OPEN; c=d='\0';

	if (argc == 1)
		fd = 0;
	else {
		fd = open(argv[1], O_RDONLY);
		if (fd == -1) {
			printf("can't open %s for reading\n", argv[1]);
			exit(0);
		}
	} 
	while (read(fd, &a, 1) == 1) {
		if (c == '?' && a == '>') {
			write(1, &a, 1);
			write(1, &nl, 1);
			continue;
		}

		if (s == 1 && a == '>')
			t=1;

		if (t == 1 && (a == '\n' || a == '\t' || a == ' '))
			continue;

		if (t == 0 && a == '\n')
			while (a == '\n' || a == '\t' || a == ' ')
				read(fd, &a, 1);

		if (c == '>' && d == '/')
			while (a == '\n' || a == '\t' || a == ' ')
				read(fd, &a, 1);

		if (a == '<') {
			t=0;
			read(fd, &b, 1);
			if (b == '/') {
				if (c == '>' && (d == '/' || pt == CLOSE))
					indent();
				write(1, &a, 1);
				write(1, &b, 1);
				pt=CLOSE;
				s=1;
				continue;
			}
			if (b == '!') {
				pt=COMMENT;
				i++;
				indent();
				i--;
			} else if (b == '?') {
				pt=VERSION;
			} else {
				pt=OPEN;
				i++;
				indent();
			}
			write(1, &a, 1);
			write(1, &b, 1);
		} else if (s == 1 && a == '>' && pt != COMMENT) {
			s=0;
			i--;
			write(1, &a, 1);
		} else
			write(1, &a, 1);

		if (a == '>' && c == '/')
			i--;
		d=c;
		c=a;
	}
	write(1, &nl, 1);
}
