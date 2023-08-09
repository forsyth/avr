#include "u.h"
#include "mem.h"
#include "os.h"
	
static char buf[64];	/* still too much */

int
snprint(char *s, int n, char *fmt, ...)
{
	return donprint(s, s+n, fmt, (&fmt+1)) - s;
}

int
print(char *fmt, ...)
{
	int n;
	byte x;

	x = splhi();
	n = donprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	uartputs(buf, n);
	splx(x);
	return n;
}
