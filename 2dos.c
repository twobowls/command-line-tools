/*
** 2dos.c    11/05/99   E. Collins
**           12/16/16	E. Collins	filter and named file modes 
**            7/17/17   E. Collins	also filter CR's not at the end of the line,
*					they are arbitrarily converted to spaces.
**
** lf to crlf converter - preserve mod and access times for named files
**
*/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>


/* These values will work under VMS */
#define GOOD 0x08000000
#define BAD  0x08000001

/* handle 8k byte lines */
#define MEMSIZ BUFSIZ*16

#ifndef O_BINARY
#define O_BINARY 0
#endif

char cr='\r';
char lf='\n';

int
convert(FILE *fpin, FILE *fpout, char *input)
{
        char buf[MEMSIZ], lastchar='\0';
	int m, x, lastcr=0, i=0;

	memset (buf, 0, MEMSIZ);
	while ((m=fread (buf, 1, MEMSIZ, fpin)) > 0) {
		if (i == 0) {
			/*
			 * One chance to discover binary input.
			 */ 
			i++;
			for (x=0; x<m; x++) {
				if (buf[x] > (char)0x7f || buf[x] == 0x0) {
					fprintf (stderr, "binary: %s\n", input);
					return (BAD);
				}
			}
		}
		for (x=0; x<m; x++) {
			if (buf[x] == (char)lf && lastchar != cr) {
				fwrite (&cr, 1, 1, fpout);
			}
			fwrite (&buf[x], 1, 1, fpout);
			if (buf[x] != (char)cr)
				lastchar='\0';
			else
				lastchar=cr;
		}

	}
	return GOOD;
}

int
main(int argc, char *argv[])
{
	char *infile;
	char tmpfile[256];
	int i = 1;
	int fd, pid;
	FILE *fp, *fptmp;
	struct stat statbuf;
	struct utimbuf utimebuf;

        if (argc == 1)
		convert(stdin, stdout, "stdin");
	else {

		pid=getpid();
		sprintf(tmpfile, "%d.tmp", pid);
		while (i < argc) {
			memset (&statbuf, 0, sizeof(statbuf));
			infile=argv[i++];
			if (stat(infile, &statbuf) == -1) {
				fprintf(stderr, "file not found: %s\n", infile);
				continue;
			}

			if ((statbuf.st_mode & S_IFDIR) == S_IFDIR) {
				fprintf(stderr, "directory: %s\n", infile);
				continue;
			}

			if ((fd=open(infile, O_RDWR | O_BINARY )) == -1) {
				fprintf(stderr, "need read and write permission on input file: %s\n", infile);
				continue;
			}
			utimebuf.modtime=statbuf.st_mtime;
			utimebuf.actime=statbuf.st_atime;

			fp=fdopen(fd, "r+");
			fptmp=fopen(tmpfile, "w");

			if (convert(fp, fptmp, infile) == BAD) {
				fclose(fptmp);
				fclose(fp);
				continue;
			}
		
			fflush(fptmp);
			fclose(fptmp);
			close (fd);
			unlink(infile);
			rename(tmpfile, infile);
			utime(infile, &utimebuf);
			fprintf(stderr, "%s\n", infile);
		}
		if (stat(tmpfile, &statbuf) == 0)
			unlink(tmpfile);
	}
	exit (GOOD);
}

