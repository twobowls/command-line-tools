/*
** hscan.c   12/21/16   E. Collins
**
** read ascii encoded hex stream on stdin, emit ascii on stdout
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int
htoi(char h)
{
	switch(h) {
	case '0': return 0x00;
	case '1': return 0x01;
	case '2': return 0x02;
	case '3': return 0x03;
	case '4': return 0x04;
	case '5': return 0x05;
	case '6': return 0x06;
	case '7': return 0x07;
	case '8': return 0x08;
	case '9': return 0x09;
	case 'a': case 'A': return 0x0a;
	case 'b': case 'B': return 0x0b;
	case 'c': case 'C': return 0x0c;
	case 'd': case 'D': return 0x0d;
	case 'e': case 'E': return 0x0e;
	case 'f': case 'F': return 0x0f;
	default: return 0x00;
	}
}

int
main(int argc, char *argv[])
{
	char buf[2];

	while (read(0, buf, 2) == 2)
		if (buf[0] > 0x20)
			printf ("%c", htoi(buf[0])*16+htoi(buf[1]));
	printf ("\n");
	exit (0);
}
