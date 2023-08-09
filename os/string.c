#include "u.h"
#include "mem.h"
#include "os.h"

int
strlen(char *s)
{
	int n;

	for(n=0; *s; s++)
		n++;
	return n;
}

int
strcmp(char *s1, char *s2)
{
	byte c1, c2;

	do{
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2){
			if(c1 > c2)
				return 1;
			return -1;
		}
	}while(c1 != 0);
	return 0;
}

int
strncmp(char *s1, char *s2, int n)
{
	byte c1, c2;

	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		n--;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			break;
	}
	return 0;
}

char*
strchr(char *s, byte c)
{
	byte sc;

	while((sc = *s++) != c)
		if(sc == 0)
			return nil;
	return s-1;
}

char*
strncpy(char *s1, char *s2, int n)
{
	int i;
	char *os1;

	os1 = s1;
	for(i = 0; i < n; i++)
		if((*s1++ = *s2++) == 0) {
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	return os1;
}
