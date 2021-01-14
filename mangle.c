/************************************************************************
**
** Command line utility for DES file encoding. 56 bit DES is obsolete
** and offers no protection against a determined opponent.  But it still
** helps honest people remain honest.
**
** Uses Phil Karn's (KA9Q) DES implementation.
**
** 1/16/2016
**         - tty raw mode for password collection
**         - preserve permissions, mod and access timestamps
**         - wildcards, "mangle *" works
**         - process only regular files, skip everything else
**         - use lockf for exclusive write on output
**         - delete input files on success
**         - don't mangle this executable
**         - enforce filename extension on mangled files
**
**       usage:
**         mangle file ...               mangle the file
**         mangle -d file ...            to decode file in place
**         mangle -o file ...            to decode file and print to stdout 
**         mangle -p password file ...   for scripting
**         mangle -h                     preach to user
**
**       build: cc -o mangle mangle.c crc.c des.c
**
***************************************************************************/

#define VERSION "1.3.4"

#ifdef WIN32
#include <time.h>
#include <winsock2.h>
#include <io.h>
#define ftruncate chsize
#define unlink remove
#else
#define O_BINARY 0
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include <sys/stat.h>
#include <utime.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <stdlib.h>   
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <termios.h>

static struct termios orig_termios;

extern int errno;
                                  
extern void endes (char [][8], char *); 
extern void dedes (char [][8], char *); 
extern void setdes (char [][8], char *);
extern void desdone (void);

struct s_t {
   unsigned int l0;
   unsigned int l1;
};

struct sval {
   unsigned int len;
   unsigned int *sbuf;
} ssval;

struct lval {
   unsigned int len;
   unsigned int slen;
   unsigned char *lbuf;
} slval;

char ks[16][8];

union uskey {
   unsigned char key[9];
   struct {
     unsigned int l0;
     unsigned int l1;
   }l;
} skey; 

#define PASSLEN  64
#define MEMSIZ  65536
#define OVERHEAD  32
#define BEGIN  0
#define MORE  1
#define END  2
#define ALL  3
#define MAGIC0  0xbeefface
#define MAGIC1  0xefbecefa

char passbuf1[PASSLEN];
char passbuf2[PASSLEN];
char outfile[512];
char infile[512];
char *EXT=".mgl";

unsigned int descrc (unsigned int len, void *buf, unsigned int *crci);
int desinit (int);
unsigned int rotateleft (unsigned int key, int rotn);
unsigned int rotatright (unsigned int key, int rotn);
char *getmpass (char buf[], int len, char *msg);
char *trimend (char *, char *);
void help (void);
void tty_atexit (void);
int tty_raw (void);
int tty_reset (void);
int oblit (int, int, off_t);
int nmchk (char *fname);

time_t setdeskey (time_t tin, union uskey pwd);
struct sval *shadow (struct lval *light, struct sval *shade, int state, union uskey pwd);
struct lval *clear (struct sval *shade, struct lval *light, int state, union uskey pwd);

int
main (int argc, char *argv[])
{
   int fdin, fdout, res, wres, state, rem, total;
   int getp, mtime, atime, ai, encode, print;
   unsigned char fbuf[MEMSIZ];
   unsigned char mbuf[MEMSIZ];
   unsigned int crc, crcn;
   struct lval *light = &slval;
   struct sval *shade = &ssval;

   int size;
   struct stat statbuf;
   struct utimbuf utimebuf;

   memset(skey.key, 0, 9);
   getp=1;
   ai=0;

   if (tcgetattr(0, &orig_termios) < 0) {
      printf("unable to get tty settings\n");
      exit(1);
   }

   if (atexit(tty_atexit) != 0) {
      printf("unable to set tty exit handler\n");
      exit(1);
   }


   encode=1, print=0;
   while (argc > 1 && argv[1][0] == '-') {
      if (argv[1][1] == 'd' || argv[1][1] == 'o') {
         encode=0;
         if (argv[1][1] == 'o')
            print=1;
         argc--;
         argv++;
         continue;
      }
      
      if (argv[1][1] == 'p' || argv[1][1] == 'P') {
         if (argc < 3 || argv[2][0] == '-') {
            printf("missing password\n");
            exit(1);
         }
         getp=0;
         strncpy(skey.key, argv[2], 8);
         skey.key[8]='\0';
         argc -= 2;
         argv += 2;
         continue;
      }

      if (argv[1][1] == 'h') {
         help();
         exit(0);
      }
      if (argv[1][1] == '\0') {
         printf("missing argument\n");
         exit(1);
      }
      if (argv[1][1] != '\0') {
         help();
         exit(0);
      }
      break;
   }
      
   if (argc == 1) {
      printf("no input files\n");
      exit(0);
   }

   while (--argc > 0) {
      memset(infile, 0, sizeof (infile));
      strncpy(infile, argv[++ai], sizeof (infile) - 1);
      infile[sizeof (infile) - 1] = '\0';

      if (stat(infile, &statbuf) == -1) {
         printf("input file not found: %s\n", infile);
         exit(1);
      }

      if (nmchk(infile) || (S_ISREG(statbuf.st_mode) == 0))
         continue;
   
      utimebuf.modtime=statbuf.st_mtime;
      utimebuf.actime=statbuf.st_atime;
   
      if ((fdin=open(infile, O_RDWR | O_BINARY )) == -1) {
         printf("need read and write permission on input file: %s\n", infile);
         exit(2);
      }

      if (getp) {
         getp=0;
         getmpass(passbuf1, PASSLEN, "enter password: ");
         getmpass(passbuf2, PASSLEN, "please re-enter password: ");
         if (strcmp(passbuf1, passbuf2) != 0) {
            printf("bad match\n");
            exit(1);
         }
         memcpy(skey.key, passbuf1, 8);
         memset(passbuf1, 0, PASSLEN);
         memset(passbuf2, 0, PASSLEN);
      }
     
      /*
       * Leave room for shadow() overhead on the first read
       */
      res=read(fdin, fbuf, MEMSIZ - OVERHEAD);
      
      shade->sbuf=(unsigned int *)fbuf;

#ifdef DEBUG
      printf("\n%s\n", infile);
      printf("magic0: 0x%x\n", ntohl(shade->sbuf[2]));
      printf("magic1: 0x%x\n", ntohl(shade->sbuf[3]));
#endif

      if (ntohl(shade->sbuf[2]) == MAGIC0 &&
                               ntohl(shade->sbuf[3]) == MAGIC1) {
   
      /*
       *
       * Decode
       *
       */
   
         if (encode == 1) {
            close(fdin);
            if (argc == 1)
               printf("file(s) are encoded, to decode use -d or -o\n");
            continue;
         }

         light->lbuf=mbuf;

         strcpy(outfile, infile);
         trimend(outfile, EXT);

         /*
          * Don't allow encoded files not named as we expect
          */
         if (strcmp(outfile, infile) == 0) {
            printf("encoded filename must end in %s\n", EXT);
            exit(3);
         }

         if (! print && (fdout=open(outfile, O_RDWR | O_CREAT | O_BINARY | O_TRUNC,
                                                                0660)) == -1) {
            printf("can't open output file for writing: %s\n", outfile);
            exit(3);
         }

         if (! print && lockf(fdout, F_TLOCK, 1) == -1) {
            printf("output file locked, aborting\n");
            exit(4);
         }
   
         if (res == statbuf.st_size) {
            state=ALL;
            shade->len=res - sizeof(unsigned int);
            light=clear(shade, light, state, skey);
            size=light->slen;
            if (! print) {
               wres=write(fdout, light->lbuf, size);
               if (wres != size) {
                  printf("can't write output file: %s\n", outfile);
                  unlink(outfile);
                  exit(5);
               }
            } else
               printf ("%s\n", light->lbuf);
            crc=descrc(size, light->lbuf, NULL);
#ifdef DEBUG
   	    printf("ALL   crc: 0x%08x, len: %d\n", crc, size);
   	    printf("sbuf: 0x%x\n",
   		shade->sbuf[shade->len / sizeof (unsigned int)]);
   	    printf("ntohl sbuf: 0x%x\n",
   		ntohl(shade->sbuf[shade->len / sizeof (unsigned int)]));
#endif
   	    if (crc != ntohl(shade->sbuf[shade->len/sizeof (unsigned int)])) {
   	       printf("wrong password given or corrupt input data\n");
               if (! print)
                  unlink(outfile);
               exit(6);
            }
         } else {
            total=0;
            state=BEGIN;
            while (1) {
               if (total + res == statbuf.st_size)
                  state=END;
               shade->len=res;
               total += res;
               if(total == statbuf.st_size)
                  shade->len -= sizeof (unsigned int);
               light=clear (shade, light, state, skey);
               if(state == BEGIN) {
                  state=MORE;
                  size=light->slen;
                  crc=descrc(light->len, light->lbuf, NULL);
#ifdef DEBUG
   	          printf("BEGIN crc: 0x%08x len: %d\n", crc, light->len);
#endif
               } else if(state == END) {
                  crc=descrc(light->len - ((size % 8) == 0 ? 0 :
                  (8 - size % 8)), light->lbuf, &crc);
#ifdef DEBUG
   	          printf("END   crc: 0x%08x len: %d\n", crc, light->len - 
   			   ((size % 8) == 0 ? 0 : (8 - size % 8)));
#endif
               } else {
                  crc=descrc(light->len, light->lbuf, &crc);
#ifdef DEBUG
                  printf("MORE  crc: 0x%08x len: %d\n", crc, light->len);
#endif
               }
               if (! print) {
                  wres=write(fdout, light->lbuf, light->len);
                  if (wres != (int)light->len) {
                     printf("can't write output file: %s\n", outfile);
                     unlink(outfile);
                     exit(5);
                  }
               } else
                  printf ("%s", light->lbuf);
               if (total == statbuf.st_size)
                  break;

               res=read(fdin, fbuf, MEMSIZ);
               if (res == -1 || res == 0) {
                  printf("can't read input file: %s %d\n", infile, errno);
                  if (! print)
                     unlink(outfile);
                  exit(7);
               }
            } /* while (1) */

            if (! print)
               ftruncate(fdout, size);
   	    if (crc != ntohl(shade->sbuf[shade->len/sizeof(unsigned int)])) {
   	       printf("input data is corrupt or wrong password\n");
               if (! print)
                  unlink(outfile);
               exit(8);
            }
         }
   
      } else {

      /*
       *
       * Encode
       *
       */
   
         if (encode == 0) {
            printf("%s is not encoded\n", infile);
            close(fdin);
            continue;
         }

         light->lbuf=fbuf;
         shade->sbuf=(unsigned int *)mbuf;

         strcpy(outfile, infile);
         strcat(outfile, EXT);
         if ((fdout=open(outfile, O_RDWR | O_CREAT | O_BINARY | O_TRUNC,
                                                                0660)) == -1) {
            printf("can't open output file for writing: %s\n", outfile);
               fflush(stdout);
            exit(3);
         }

         if (lockf(fdout, F_TLOCK, 1) == -1) {
            printf("output file locked, aborting\n");
            exit(4);
         }
   
         crc=descrc(res, fbuf, NULL);
         if (res == statbuf.st_size) {
#ifdef DEBUG
            printf("ALL   crc: 0x%08x, len: %d\n", crc, res);
#endif
            crcn=htonl (crc);
            crc=crcn;
            state=ALL;
            light->len=res;
            light->slen=statbuf.st_size;
            shade=shadow(light, shade, state, skey);
            wres=write(fdout, shade->sbuf, shade->len);
            wres += write(fdout, &crc, sizeof(unsigned int));
            if (wres != (int)shade->len + (int)sizeof(unsigned int)) {
               printf("can't write output file: %s\n", outfile);
               unlink(outfile);
               exit(5);
            }

         } else {
   
            state= BEGIN;
            total=0;
            light->slen=statbuf.st_size;
            while (1) {
               light->len=res;
               shade=shadow (light, shade, state, skey);
               if (state != BEGIN)
                  crc=descrc(res, fbuf, &crc);
#ifdef DEBUG
               else
                  printf("BEGIN crc: 0x%08x, len: %d\n", crc, res);
#endif
               wres=write(fdout, shade->sbuf, shade->len);
               if (wres != (int)shade->len) {
                  printf("can't write output file: %s\n", outfile);
                  unlink(outfile);
                  exit(5);
               }

               total += res;
               if (total == statbuf.st_size) {
#ifdef DEBUG
   	          printf("END   crc: 0x%x, len: %d\n", crc, wres);
#endif
   	          crcn=htonl(crc);
   	          crc=crcn;
                  wres += write(fdout, &crc, sizeof(unsigned int));
                  if (wres != (int)shade->len + (int)sizeof(unsigned int)) {
                     printf("can't write output file: %s\n", outfile);
                     unlink(outfile);
                     exit(5);
                  }
                  break;
               }
#ifdef DEBUG
               if (state != BEGIN)
                  printf("MORE  crc: 0x%08x, len: %d\n", crc, res);
#endif
               state=MORE;
               res=read(fdin, fbuf, MEMSIZ);
               if (res == -1 || res == 0) {
                  printf("can't read input file: %s %d\n", infile, errno);
                  unlink(outfile);
                  exit(5);
               }
            }      /* while total != statbuf.st_size */
         }
      }

      if (encode)
         oblit(fdout, fdin, statbuf.st_size);
      desdone();
      close(fdin);
#ifdef DEBUG
      printf("infile:  %s\n", infile);
#endif
      if (! print) {
         fchmod(fdout, statbuf.st_mode);
         fsync(fdout);
         close(fdout);
         utime(outfile, &utimebuf);
         unlink(infile);
         printf("%s\n", outfile);
      }
   }
   exit(0);
}


/*
 * Hammer the original file before deleting.
 * This doesn't accomplish much with solid
 * state drives.
 */
int
oblit(int in, int out, off_t size)
{
   unsigned char buf[512];
   unsigned char buf2[512];
   int i, x=0;
   off_t off=0, oof;

   lseek(in, off, SEEK_SET);
   oof=lseek(out, off, SEEK_SET);

   while (x < size) {
      i=read(in, buf, 512);
      if (i < 0)
         perror("read: ");
      i=write(out, buf, 512);
      if (i < 0)
         perror("write: ");
      x += MEMSIZ;
   }
   fsync(out);
   memset(buf2, 0xA5, 512);
   lseek(in, off, SEEK_SET);
   lseek(out, off, SEEK_SET);
   while (read (in, buf, 512) > 0)
      write(out, buf2, 512);
   fsync(out);
   memset(buf2, 0x5A, 512);
   lseek(in, off, SEEK_SET);
   lseek(out, off, SEEK_SET);
   while (read (in, buf, 512) > 0)
      write(out, buf2, 512);
   fsync(out);
   return x;
}


char *
trimend (char *instr, char *trim)
{
   int x, y;
   x=strlen(instr);
   y=strlen(trim);

   if (x < y+1)
      return instr;

   x -= y;
   while (x>=0) {
      if (instr[x] == trim[0]) {
         if (! strcmp(&instr[x], trim)) {
            instr[x]='\0';
            break;
         }
      }
      x--;
   }
   return instr;
}

int
nmchk (char *fname)
{
   const char *nm1="mangle";
   const char *nm2="mangle.exe";
   char *fsep;
   int i;

   if (fname == NULL)
      return 0;

   i=strlen(fname);
   if (i < 5)
      return 0;

   fsep=fname + i;
   while (--fsep != fname) {
      if (fsep[0] == '\\' || fsep[0] == '/') {
         fsep++;
         break;
      }
   } 

   if (strcmp(fsep, nm1) == 0 || strcmp(fsep, nm2) == 0)
      return 1;

   return 0;
}

void
help (void)
{
   printf("\n\n\nmangle %s is a file privacy utility.\n", VERSION);
   printf("\nEncoded files can not be recovered if the password is lost.\n");
   printf("\nDuring encoding files are renamed as filename.mgl. When ");
   printf("decoding (-d|-o) mangle\ninsists on this naming convention and ");
   printf("will refuse to decode files that have been\nrenamed ");
   printf("without this extension.\n");
   printf("\nWildcards are supported, take care when using them!\n");
   printf("\nFile permissions, access times and modification times ");
   printf("are preserved across\nencoding/decoding events.\n");
   printf("\nOriginal files are overwritten and deleted except when\n");
   printf("using -o to output encoded content to stdout.\n"); 
   printf("\nWhen using -o be careful to avoid observation by others,\n"); 
   printf("as the entire contents of encoded file(s) will be printed to stdout.\n");
   printf("\nEncoded files are slightly larger than the original.\n");
   printf("\nUsage: mangle -h\n");
   printf("       mangle [-d|-o] [-p password] file [file ...]\n\n");
}

void
tty_atexit(void)
{
   tty_reset();
}

int
tty_raw()
{
   struct termios raw;

   memcpy(&raw, &orig_termios, sizeof(struct termios));
   cfmakeraw(&raw);
   if (tcsetattr(0, TCSAFLUSH, &raw) < 0)
      return -1;
   return 0;
}

int
tty_reset(void)
{
   if (tcsetattr(0, TCSAFLUSH, &orig_termios) < 0)
      return -1;
   return 0;
}

char *
getmpass (char buf[], int len, char *msg)
{
   int i=0;
   unsigned char a;

   memset(buf, 0, len);
   write(0, msg, strlen (msg));
   tty_raw();
   while (read (0, &a, 1) && i < PASSLEN-1) {
      if (a == 0x0a || a == 0x0d)
         break;
      buf[i] = a;
      i++;
   }
   tty_reset();
   a='\r';
   write(0, &a, 1);
   a='\n';
   write(0, &a, 1);
   
   buf[i]='\0';
   return buf;
}

/*
 * Barrel roll left.
 */
unsigned int
rotateleft (unsigned int key, int rotn)
{
   while (rotn--) {
      if (key & 0x80000000) {
         key=key << 1;
         key |= 01;
      } else
         key=key << 1;
   }
   return key;
}


/*
 * Barrel roll right.
 */
unsigned int
rotateright (unsigned int key, int rotn)
{
   while (rotn--) {
      if (key & 01) {
         key=key >> 1;
         key |= 0x80000000;
      } else
         key=key >> 1;
   }
   return key;
}

/*
 * This function uses a time-based derivation of a fixed key to
 * produce a key which does not repeat even if the user provides
 * the same password for multiple sessions.
 */
time_t
setdeskey(time_t tin, union uskey pwd)
{
   struct s_t s;
   int rotn;
   time_t tm;

   if (tin)
      tm=tin;
   else
      tm=htonl(time (NULL));
   rotn=ntohl(tm) % 32;
   if (rotn < 0)
      rotn *= -1;

#ifdef DEBUG
   printf("tm: 0x%x\n", (unsigned int)tm);
   printf("rotn: %d\n", rotn);
#endif

   s.l0=0xa52b3f9e;
   s.l0=rotateleft(s.l0, rotn);
   s.l0=htonl(s.l0) ^ tm;
   s.l0=s.l0 ^ pwd.l.l0;

   s.l1=0x7de29ca3;
   s.l1=rotateright(s.l1, rotn);
   s.l1=htonl(s.l1) ^ tm;
   s.l1=s.l1 ^ pwd.l.l1;

   desinit(0);
   setdes(ks, (char *)&s);
   return tm;
}


/*
 * Convert clear data to encoded data. Repeat encodings of the
 * same source data yield vastly different encoded data.
 */
struct sval *
shadow (struct lval *light, struct sval *shade, int state, union uskey pwd)
{
   int rounds, remainder, iter, count;
   unsigned int *ubuf, pmagic[2];
        
   struct s_t s;

   count=0;
   pmagic[0]=htonl(MAGIC0);
   pmagic[1]=htonl(MAGIC1);
   shade->len=((light->len  + sizeof(int) - 1) / sizeof(int));
   if (shade->len & 1)
      shade->len++;

   if (state == BEGIN || state == ALL)
      shade->len += 4;

   shade->len *= sizeof (int);
   ubuf=(unsigned int *)shade->sbuf;

   if (state == BEGIN || state == ALL) {
      ubuf[0]=setdeskey (0L, pwd);
      ubuf[1]=htonl((unsigned)light->slen);
      ubuf[2]=htonl(MAGIC0);
      ubuf[3]=htonl(MAGIC1);
      iter=4;
   } else
      iter=0;

   rounds=light->len / 8;
   remainder=light->len % 8;
   while (rounds--) {
      memset(&s, 0, sizeof (s));
      memcpy(&s, &light->lbuf[count * 8], 8);
      endes(ks, (char *)&s); 
      ubuf[iter]=s.l0;
      ubuf[1 + iter]=s.l1;
      iter += 2;
      count++;
   }

   if (remainder) {
      memset(&s, 0, sizeof (s));
      memcpy(&s, &light->lbuf[count * 8], remainder);
      endes(ks, (char *)&s); 
      ubuf[iter]=s.l0;
      ubuf[1 + iter]=s.l1;
   }
   return shade;
}

/*
 * Convert encoded data back to clear.
 */
struct lval *
clear (struct sval *shade, struct lval *light, int state, union uskey pwd)
{
   int rounds, remainder, iter, count;
   struct s_t t;

   if (state == BEGIN || state == ALL) {
      setdeskey(shade->sbuf[0], pwd);
      light->slen=ntohl(shade->sbuf[1]);
      light->len=shade->len - 16;
      iter=4;
   } else {
      light->slen=0;
      light->len=shade->len;
      iter=0;
   }

   memset(light->lbuf, 0, MEMSIZ);
   rounds=light->len / 8;
   remainder=light->len % 8;
   count=0;
   while (rounds--) {
      t.l0=shade->sbuf[iter];
      t.l1=shade->sbuf[1 + iter];
      dedes(ks, (char *)&t);
      memcpy(&light->lbuf[count * 8], &t, 8);
      iter += 2;
      count++;
   }
   if (remainder) {
      t.l0=shade->sbuf[iter];
      t.l1=shade->sbuf[1 + iter];
      dedes(ks, (char *)&t);
      memcpy(&light->lbuf[count * 8], &t, 8);
   }
   return light;
}
