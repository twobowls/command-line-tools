/************************************************************************
**
** oblit.c
**
** User command for erasing files
**
** It worked ok back in the day, fairly useless now.
**
** 13-MAY-1999  Efton Collins     first written
**
***************************************************************************/

#ifdef __STDC__
#define _P(args) args
#else
#define _P(args) ()
#endif

#ifdef WIN32

#include <time.h>
#include <io.h>
#define unlink remove

#else

#define O_BINARY 0
#include <sys/types.h>
#include <unistd.h>

#endif


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <stdlib.h>   
#include <stdio.h>
#include <string.h>

extern int errno;
#define MEMSIZ BUFSIZ*10

int
main (argc, argv)
int argc; char *argv[];
{
   int fd, argn;
   char *file;
   unsigned char fbuf[MEMSIZ];
   long nwritten;
   struct stat statbuf;

   if (argc == 1) {
      printf ("usage: %s filename [filename] ...\n", argv[0]);
      printf ("warning: oblit overwrites then deletes your file - "
	 "make sure you mean it\n");
      printf ("         filename expansion is performed,"
			 " watch those wildcards!\n");
      printf ("         oblit * is especially destructive.\n");
      return 1;
   }
 
   memset (fbuf, 0, MEMSIZ);
   argn = 1;
   while (--argc) {
      file = argv[argn++];
      printf ("%s\n", file);
      if (stat (file, &statbuf) == -1) {
         printf ("can't find file: %s\n", file);
         continue;
      }

      if ((fd = open (file, O_RDWR | O_BINARY )) == -1) {
         printf ("can't open file for writing: %s\n", file);
         continue;
      }
   
      nwritten = 0;
      while (nwritten != statbuf.st_size) {
         if (statbuf.st_size - nwritten >= MEMSIZ)
            nwritten += write (fd, fbuf, MEMSIZ);
         else
            nwritten += write (fd, fbuf, statbuf.st_size - nwritten);
      }

      close (fd);
      unlink (file);
   }
   return 0;
}
