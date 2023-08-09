#include <u.h>
#include <libc.h>
#include <bio.h>

Biobuf	bin;

void
main(int argc, char **argv)
{
	char *p, last[200], *fields[5],  *tag;
	int base, n;

	tag = "lLtT";	/* text */
	ARGBEGIN{
	case 'd':
		tag = "dD";
		break;
	case 'b':
		tag = "bB";
		break;
	}ARGEND

	Binit(&bin, 0, OREAD);
	base = -1;
	while((p = Brdline(&bin, '\n')) != 0){
		p[Blinelen(&bin)-1] = 0;
		n = getfields(p, fields, nelem(fields), 1, " \t\n");
		if(n < 3)
			continue;
		if(strchr(tag, fields[1][0]) == nil)
			continue;
		n = strtoul(fields[0], 0, 16);
		if(base >= 0)
			fprint(1, "%8d %s\n", n-base, last);
		base = n;
		strcpy(last, fields[2]);
	}
	exits(0);
}
