/*
 * len.c
 *
 * usage: len [-t] <file> [len]
 * print a list showing each distinct length of lines in <file>,
 * and the number of occurrences.
 * if argument [len] is given, print the line(s) having length [len].
 * if the -t switch is given, expand tabs to 8 characters.
 *
 * E. Collins   19-Nov-1997
 * E. Collins	17-Apr-2004	add tab expand switch
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* key type */
typedef unsigned long data_t;

struct tag_list {
   struct tag_list * next;
   struct tag_list * prev;
   data_t key;
   data_t data;
} ;

typedef struct tag_list LINK;

void   listDrop (void);              /* delete entire list */
LINK * listMem (void);               /* allocate new link element    */
int    listAdd (LINK *);             /* add new data element to list */
int    listDel (data_t);             /* delete a list element        */
int    listCount (void);             /* return count list elements   */
LINK * listSearch (data_t);          /* simple sequencial search     */
LINK * listBinSearch (data_t, int);  /* binary search of linked list */

/* reentrant version of strtok() */
char *strtokp (char *, char *, char **);

unsigned long calcCRC(unsigned int, void *);


void   listWalkContent();

/* figure out how long the expanded tabs would be. */
data_t doTab(char *);

#define GOOD 0x08000000
#define BAD  0x08000001


int
main(int argc, char *argv[])
{
	FILE *fp;
	LINK *p;
	char buf[1024*1024];
	data_t len,n;
	int dotab = 0;

	if (argc == 1) {
		printf ("Usage: %s [-t] <file> [len]\n", argv[0]);
		printf ("use -t switch to expand tabs\n");
		exit (BAD);
	}

	if (argv[1][0] == '-' && argv[1][1] == 't') {
		dotab=1;
		argc--;
	}

	if ((fp = fopen (argv[1+dotab], "r")) == NULL) {
		printf ("%s: can't open\n", argv[1+dotab]);
		exit(BAD);
	}

	if (argc == 3)
		n = (data_t)atoi (argv[2+dotab]);
	else
		n = 0;
		
	while (fgets (buf, sizeof(buf), fp)) {
		if (dotab>0)
			len = doTab(buf);
		else
			len = (data_t)strlen (buf)-1;

		
		if (n != 0 && n == len)
			printf ("%s", buf);
		if ((p = listSearch (len)) == NULL) {
			p = listMem();
			if (!p) {
				printf ("no mem\n");
				exit(BAD);
			}
			p->data = 1;
			p->key = len;
			listAdd (p);
		} else 
			++p->data;
	}
	fclose (fp);
	if (!n) 
		listWalkContent(); 
	exit(GOOD);
}

data_t
doTab(char *buf)
{
	data_t tablen = 0;
	int loc;

	for (loc=0; buf[loc] != '\0' && buf[loc] != '\n'; loc++) {
		if (buf[loc] == '\t') {
			// printf ("tablen: %d, tablen%8: %d\n", tablen, tablen%8);
			if (tablen < 8)
				tablen += (data_t)(8-tablen);
			else
				tablen += (data_t)8-(tablen%8);
		} else
			tablen++;
	}
	return tablen;
}

/*************************************************************************
    FUNCTION:  strtokp (char *, char *, char **)
 
    PURPOSE: strtokp () is a reentrant version of strtok.
             Instead of storing the pointer to the location in the string
             to begin the next search this version returns the location to
             the caller.  Restricted to single character delimiters.
 
             Returns pointer to delimited substring, also returns pointer
             to next search location in the last argument.
 
    06/21/96   first written                          E. Collins
**************************************************************************/
char *
strtokp (char *in, char *delim, char **out)
{
    int i = 0;
    char *inp = in;
 
    if (!in || !delim) 
    {
       *out = NULL;
       return NULL;
    }
 
    while (in[i] != delim[0] && in[i])
       i++;
 
    if (!in[i]) 
    {
       *out = NULL;
       return inp;
    }
 
    in[i++] = '\0';
    *out = in + i;
    return inp; 
}

/*********************************************************************
**
**
** Support of sorted linked lists - binary search of linked lists
**
** 01/31/96     Added support for ordered inserts         Efton Collins
** 02/21/96     Added NULL head/tail check in bin search  Efton Collins
**
***********************************************************************/  

/* head, tail, count - linked list support elements -private this file */
static LINK *head = NULL;
static LINK *tail = NULL;
static int count = 0;



/*
 * Binary search on sorted doubly linked list
 */
LINK *
listBinSearch (data_t data, int reset)
{
   static LINK **s = NULL;
   static int firsttime = 1;
   LINK *p = head;

   int min = 0;
   int mid;
   int max = listCount() - 1;
   int now = 0;

   if (reset) {
      firsttime = 1;
      if (s)
         free (s);
      return NULL;
   }

   if (head == NULL || tail == NULL) { /* There is no list */
      printf ("panic: no list\n");
      return NULL;
   }

   if (firsttime && (s = (LINK **)malloc (sizeof (LINK *) * listCount()))
                                                                   == NULL) {
      printf ("panic: can't malloc\n");
      return NULL;
   }

   /*
    * Initialize our array of element pointers - needed because list
    * elements are not contiguous.
    */
   if (firsttime) {
      firsttime = 0;
      while (p->next != NULL) {
         s[now++] = p;
         p = p->next;
      }
      s[now] = p;
   }

   /*
    * Binary search array by key.
    */
   while (min <= max) {
      mid = (min + max) / 2;
      if (data < s[mid]->key)
         max = mid - 1;
      else if (data > s[mid]->key)
         min = mid + 1;
      else
         return s[mid];
   }
   return NULL;
}


void
listDrop(void)
{
   LINK *p = head, *n;
   int freecount = 0;

   if (p == NULL) {
      count = 0;
      return;
   }

   while (n = p->next) {
      free (p);
      freecount++;
      p = n;
   }
   free (p);
   freecount++;
   head = tail = NULL;
   count = 0;
   return;
}



int
listCount(void)
{
   return count;
}


/*
 * Returns: 
 * success            0
 * head null         -1
 * bad head          -2
 * element not found -3
 */
int
listDel (data_t data)
{
   LINK *p = head;

   if (p == NULL)
      return -1;

   /* sequential search */
   while (p->key != data && p->next != NULL)
      p = p->next;

   if (p->key == data) {
      if (p->next == NULL && p->prev == NULL) {
         if (p != head)                             /* integrity check */ 
            return -2;
         free (p);
         count = 0;
         head = NULL;                               /* no more nodes */
         tail = NULL;
         return 0;
      }
      if (p->next == NULL && p->prev != NULL) {
         (p->prev)->next = NULL;                    /* end of list */
         tail = p->prev;
      }
      if (p->next != NULL && p->prev == NULL) {
         (p->next)->prev = NULL;                    /* beginning of list */
         head = p->next;
      }
      if (p->next != NULL && p->prev != NULL) {
         (p->next)->prev = p->prev;
         (p->prev)->next = p->next;                 /* internal node */
      }
      free (p);
      count--;
      return 0;
   }
   return -3;
}


/*
 * Returns: 
 * success            0
 * malloc error      -1
 * duplicate key     -2
 * duplicate element -3
 * NULL element      -4
 */
int 
listAdd (LINK * new)
{
   LINK *p = head;


 /*
  * Make sure element passed in is not NULL
  */
   if (new == NULL)
      return (-4);

 /*
  * new list
  */
   if (head == NULL) {
      head = new;
      tail = head;
      head->prev = NULL;
      head->next = NULL;
      count = 1;
      return 0;
   }

 /*
  * scan for duplicated element
  */
   while (p->next != NULL) {
      if (p == new)
          return -3;
      p = p->next;
   }
   if (p == new)
      return -3;
   p = head;

 /*
  * navigate to the site
  */
   while (p->key < new->key) {
      if (p->next != NULL)
         p = p->next;
      else
         break;
   }

 /*
  * duplicate key check
  */
   if (new->key == p->key)
         return -2;

 /*
  * add element to end of list
  */
   if (p->next == NULL && p->key < new->key ) {  /* we're at the end */
      p->next = new; 
      new->prev = p;
      new->next = NULL;
      tail = new;
      count++;
      return 0;
   }

   new->next = p;
   new->prev = p->prev;
   if (p->prev != NULL) /* not head */
      (p->prev)->next = new;
   else
      head = new;
   p->prev = new;
   count++;
   return 0;
}



void
listWalkContent (void)
{
   LINK *p = head, *t = tail;

   if (p == NULL || t == NULL) {
      printf ("%s is NULL\n", p == NULL ? "Head" : "Tail");
      return;
   }

   while (p->next != NULL) {
      printf ("len %4d, count %4d\n", p->key, p->data);
      p = p->next;
   }
   printf ("len %4d, count %4d\n", p->key, p->data);
}
     

LINK *
listMem (void)
{
   LINK *pLink;
   pLink = (LINK *)malloc (sizeof (LINK));
   if (pLink != NULL)
      memset (pLink, 0, sizeof (LINK));
   return pLink;
}


LINK *
listSearch (data_t data)
{
   LINK *p = head;

   if (p == NULL)
      return p;

   while (p->key != data && p->next != NULL)
      p = p->next;
   if (p->key != data)
      return NULL;
   return p;
}

