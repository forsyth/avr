#define	EXTERN
#include	"l.h"
#include	<ar.h>

#ifndef	DEFAULT
#define	DEFAULT	'9'
#endif

char	*noname		= "<none>";
char	symname[]	= SYMDEF;
char	thechar		= 'z';
char	*thestring 	= "avr";

/*
 *	-H1 -T0 -D0 -R0		is raw executable, no header, separate i&d
 */

static int reguse;
static int stkusepc, stkusefp;

void
main(int argc, char *argv[])
{
	int c;
	char *a;

	Binit(&bso, 1, OWRITE);
	cout = -1;
	listinit();
	outfile = 0;
	nerrors = 0;
	curtext = P;
	zversion = V2;
	HEADTYPE = 2;
	INITTEXT = 0;
	INITDAT = 0x100;
	INITRND = 0;
	INITENTRY = 0;

	ARGBEGIN {
	default:
		c = ARGC();
		if(c >= 0 && c < sizeof(debug))
			debug[c]++;
		break;
	case 'o':
		outfile = ARGF();
		break;
	case 's':
		stkusepc = 1;
		break;
	case 'S':
		stkusefp = 1;
		break;
	case 'E':
		a = ARGF();
		if(a)
			INITENTRY = a;
		break;
	case 'T':
		a = ARGF();
		if(a)
			INITTEXT = atolwhex(a);
		break;
	case 'D':
		a = ARGF();
		if(a)
			INITDAT = atolwhex(a);
		break;
	case 'R':
		a = ARGF();
		if(a)
			INITRND = atolwhex(a);
		break;
	case 'H':
		a = ARGF();
		if(a)
			HEADTYPE = atolwhex(a);
		break;
	case 'V':
		a = ARGF();
		if(a)
			zversion = atolwhex(a);
		break;
	} ARGEND
	USED(argc);
	if(*argv == 0) {
		diag("usage: zl [-options] objects");
		errorexit();
	}
	if(!debug['9'] && !debug['U'] && !debug['B'])
		debug[DEFAULT] = 1;
	if(HEADTYPE == -1) {
		if(debug['U'])
			HEADTYPE = 0;
		if(debug['B'])
			HEADTYPE = 1;
		if(debug['9'])
			HEADTYPE = 2;
	}
	switch(HEADTYPE) {
	default:
		diag("unknown -H option");
		errorexit();

	case 1:	/* raw */
		HEADR = 0;
		if(INITTEXT == -1)
			INITTEXT = 0;
		if(INITDAT == -1) {
			INITDAT = 0;
			INITRND = 2;
		}
		if(INITRND == -1)
			INITRND = 0;
		break;

	case 2:
		HEADR = 32L;
		break;
	}
	if(INITDAT != 0 && INITRND != 0)
		print("warning: -D0x%lux is ignored because of -R0x%lux\n",
			INITDAT, INITRND);
	if(debug['v'])
		Bprint(&bso, "HEADER = -H0x%x -T0x%lux -D0x%lux -R0x%lux\n",
			HEADTYPE, INITTEXT, INITDAT, INITRND);
	Bflush(&bso);
	zprg.as = AGOK;
	zprg.size = 0;
	zprg.from.name = D_NONE;
	zprg.from.type = D_NONE;
	zprg.from.reg = NREG;
	zprg.to = zprg.from;
	buildop();
	histgen = 0;
	textp = P;
	datap = P;
	pc = 0;
	dtype = 4;
	if(outfile == 0)
		outfile = "z.out";
	cout = create(outfile, 1, 0775);
	if(cout < 0) {
		diag("%s: cannot create", outfile);
		errorexit();
	}
	nuxiinit();
	version = 0;
	cbp = buf.cbuf;
	cbc = sizeof(buf.cbuf);
	firstp = prg();
	lastp = firstp;

	if(INITENTRY == 0) {
		INITENTRY = "_startup";
		if(debug['p'])
			INITENTRY = "_startupp";
		if(!debug['l'])
			lookup(INITENTRY, 0)->type = SXREF;
	} else
		lookup(INITENTRY, 0)->type = SXREF;

	while(*argv)
		objfile(*argv++);
	if(!debug['l'])
		loadlib();
	firstp = firstp->link;
	if(firstp == P)
		goto out;
	patch();
	if(debug['p'])
		if(debug['1'])
			doprof1();
		else
			doprof2();
	dodata();
	follow();
	if(firstp == P)
		goto out;
	noops();
	if(reguse)
		asmr();
	if(stkusepc || stkusefp)
		asms(stkusepc);
	span();
	asmb();
	undef();

out:
	if(debug['v']) {
		Bprint(&bso, "%5.2f cpu time\n", cputime());
		Bprint(&bso, "%ld memory used\n", tothunk);
		Bprint(&bso, "%d sizeof adr\n", sizeof(Adr));
		Bprint(&bso, "%d sizeof prog\n", sizeof(Prog));
	}
	errorexit();
}

void
loadlib(void)
{
	int i;
	long h;
	Sym *s;

loop:
	xrefresolv = 0;
	for(i=0; i<libraryp; i++) {
		if(debug['v'])
			Bprint(&bso, "%5.2f autolib: %s (from %s)\n", cputime(), library[i], libraryobj[i]);
		objfile(library[i]);
	}
	if(xrefresolv)
	for(h=0; h<nelem(hash); h++)
	for(s = hash[h]; s != S; s = s->link)
		if(s->type == SXREF)
			goto loop;
}

void
errorexit(void)
{

	Bflush(&bso);
	if(nerrors) {
		if(cout >= 0)
			remove(outfile);
		exits("error");
	}
	exits(0);
}

void
objfile(char *file)
{
	long off, esym, cnt, l;
	int f, work;
	Sym *s;
	char magbuf[SARMAG];
	char name[100], pname[150];
	struct ar_hdr arhdr;
	char *e, *start, *stop;

	if(file[0] == '-' && file[1] == 'l') {
		if(debug['9'])
			sprint(name, "/%s/lib/lib", thestring);
		else
			sprint(name, "/usr/%clib/lib", thechar);
		strcat(name, file+2);
		strcat(name, ".a");
		file = name;
	}
	if(debug['v'])
		Bprint(&bso, "%5.2f ldobj: %s\n", cputime(), file);
	Bflush(&bso);
	f = open(file, 0);
	if(f < 0) {
		diag("cannot open file: %s", file);
		errorexit();
	}
	l = read(f, magbuf, SARMAG);
	if(l != SARMAG || strncmp(magbuf, ARMAG, SARMAG)){
		/* load it as a regular file */
		l = seek(f, 0L, 2);
		seek(f, 0L, 0);
		ldobj(f, l, file);
		close(f);
		return;
	}

	l = read(f, &arhdr, SAR_HDR);
	if(l != SAR_HDR) {
		diag("%s: short read on archive file symbol header", file);
		goto out;
	}
	if(strncmp(arhdr.name, symname, strlen(symname))) {
		diag("%s: first entry not symbol header", file);
		goto out;
	}

	esym = SARMAG + SAR_HDR + atolwhex(arhdr.size);
	off = SARMAG + SAR_HDR;

	/*
	 * just bang the whole symbol file into memory
	 */
	seek(f, off, 0);
	cnt = esym - off;
	start = malloc(cnt + 10);
	cnt = read(f, start, cnt);
	if(cnt <= 0){
		close(f);
		return;
	}
	stop = &start[cnt];
	memset(stop, 0, 10);

	work = 1;
	while(work){
		if(debug['v'])
			Bprint(&bso, "%5.2f library pass: %s\n", cputime(), file);
		Bflush(&bso);
		work = 0;
		for(e = start; e < stop; e = strchr(e+5, 0) + 1) {
			s = lookup(e+5, 0);
			if(s->type != SXREF)
				continue;
			sprint(pname, "%s(%s)", file, s->name);
			if(debug['v'])
				Bprint(&bso, "%5.2f library: %s\n", cputime(), pname);
			Bflush(&bso);
			l = e[1] & 0xff;
			l |= (e[2] & 0xff) << 8;
			l |= (e[3] & 0xff) << 16;
			l |= (e[4] & 0xff) << 24;
			seek(f, l, 0);
			l = read(f, &arhdr, SAR_HDR);
			if(l != SAR_HDR)
				goto bad;
			if(strncmp(arhdr.fmag, ARFMAG, sizeof(arhdr.fmag)))
				goto bad;
			l = atolwhex(arhdr.size);
			ldobj(f, l, pname);
			if(s->type == SXREF) {
				diag("%s: failed to load: %s", file, s->name);
				errorexit();
			}
			work = 1;
			xrefresolv = 1;
		}
	}
	return;

bad:
	diag("%s: bad or out of date archive", file);
out:
	close(f);
}

int
zaddr(uchar *p, Adr *a, Sym *h[])
{
	int i, c;
	long l;
	Sym *s;
	Auto *u;

	c = p[2];
	if(c < 0 || c > NSYM){
		print("sym out of range: %d\n", c);
		p[0] = ALAST+1;
		return 0;
	}
	a->type = p[0];
	a->reg = p[1];
	a->sym = h[c];
	a->name = p[3];
	c = 4;

	if(a->reg > NREG) {
		print("register out of range %d\n", a->reg);
		p[0] = ALAST+1;
		return 0;	/*  force real diagnostic */
	}

	switch(a->type) {
	default:
		print("unknown type %d\n", a->type);
		p[0] = ALAST+1;
		return 0;	/* force real diagnostic */

	case D_NONE:
	case D_REG:
	case D_FREG:
	case D_SREG:
	case D_PORT:
	case D_PREREG:
	case D_POSTREG:
	case D_RAMPD:
	case D_RAMPY:
	case D_RAMPZ:
		break;

	case D_OREG:
	case D_BRANCH:
	case D_CONST:
		a->offset = p[4] | (p[5]<<8) |
			(p[6]<<16) | (p[7]<<24);
		c += 4;
		break;

	case D_SCONST:
		memmove(a->sval, p+4, NSNAME);
		c += NSNAME;
		break;

	case D_FCONST:
		a->ieee.l = p[4] | (p[5]<<8) |
			(p[6]<<16) | (p[7]<<24);
		a->ieee.h = p[8] | (p[9]<<8) |
			(p[10]<<16) | (p[11]<<24);
		c += 8;
		break;
	}
	s = a->sym;
	if(s == S)
		goto out;
	i = a->name;
	if(i != D_AUTO && i != D_PARAM)
		goto out;

	l = a->offset;
	for(u=curauto; u; u=u->link)
		if(u->asym == s)
		if(u->type == i) {
			if(u->aoffset > l)
				u->aoffset = l;
			goto out;
		}

	u = malloc(sizeof(Auto));

	u->link = curauto;
	curauto = u;
	u->asym = s;
	u->aoffset = l;
	u->type = i;
out:
	return c;
}

void
addlib(char *obj)
{
	char name[1024], comp[256], *p;
	int i;

	if(histfrogp <= 0)
		return;

	if(histfrog[0]->name[1] == '/') {
		sprint(name, "");
		i = 1;
	} else
	if(histfrog[0]->name[1] == '.') {
		sprint(name, ".");
		i = 0;
	} else {
		if(debug['9'])
			sprint(name, "/%s/lib", thestring);
		else
			sprint(name, "/usr/%clib", thechar);
		i = 0;
	}

	for(; i<histfrogp; i++) {
		snprint(comp, sizeof comp, histfrog[i]->name+1);
		for(;;) {
			p = strstr(comp, "$O");
			if(p == 0)
				break;
			memmove(p+1, p+2, strlen(p+2)+1);
			p[0] = thechar;
		}
		for(;;) {
			p = strstr(comp, "$M");
			if(p == 0)
				break;
			if(strlen(comp)+strlen(thestring)-2+1 >= sizeof comp) {
				diag("library component too long");
				return;
			}
			memmove(p+strlen(thestring), p+2, strlen(p+2)+1);
			memmove(p, thestring, strlen(thestring));
		}
		if(strlen(name) + strlen(comp) + 3 >= sizeof(name)) {
			diag("library component too long");
			return;
		}
		strcat(name, "/");
		strcat(name, comp);
	}
	for(i=0; i<libraryp; i++)
		if(strcmp(name, library[i]) == 0)
			return;
	if(libraryp == nelem(library)){
		diag("too many autolibs; skipping %s", name);
		return;
	}

	p = malloc(strlen(name) + 1);
	strcpy(p, name);
	library[libraryp] = p;
	p = malloc(strlen(obj) + 1);
	strcpy(p, obj);
	libraryobj[libraryp] = p;
	libraryp++;
}

void
addhist(long line, int type)
{
	Auto *u;
	Sym *s;
	int i, j, k;

	u = malloc(sizeof(Auto));
	s = malloc(sizeof(Sym));
	s->name = malloc(2*(histfrogp+1) + 1);

	u->asym = s;
	u->type = type;
	u->aoffset = line;
	u->link = curhist;
	curhist = u;

	j = 1;
	for(i=0; i<histfrogp; i++) {
		k = histfrog[i]->value;
		s->name[j+0] = k>>8;
		s->name[j+1] = k;
		j += 2;
	}
}

void
histtoauto(void)
{
	Auto *l;

	while(l = curhist) {
		curhist = l->link;
		l->link = curauto;
		curauto = l;
	}
}

void
collapsefrog(Sym *s)
{
	int i;

	/*
	 * bad encoding of path components only allows
	 * MAXHIST components. if there is an overflow,
	 * first try to collapse xxx/..
	 */
	for(i=1; i<histfrogp; i++)
		if(strcmp(histfrog[i]->name+1, "..") == 0) {
			memmove(histfrog+i-1, histfrog+i+1,
				(histfrogp-i-1)*sizeof(histfrog[0]));
			histfrogp--;
			goto out;
		}

	/*
	 * next try to collapse .
	 */
	for(i=0; i<histfrogp; i++)
		if(strcmp(histfrog[i]->name+1, ".") == 0) {
			memmove(histfrog+i, histfrog+i+1,
				(histfrogp-i-1)*sizeof(histfrog[0]));
			goto out;
		}

	/*
	 * last chance, just truncate from front
	 */
	memmove(histfrog+0, histfrog+1,
		(histfrogp-1)*sizeof(histfrog[0]));

out:
	histfrog[histfrogp-1] = s;
}

void
nopout(Prog *p)
{
	p->as = ANOOP;
	p->from.type = D_NONE;
	p->to.type = D_NONE;
}

uchar*
readsome(int f, uchar *buf, uchar *good, uchar *stop, int max)
{
	int n;

	n = stop - good;
	memmove(buf, good, stop - good);
	stop = buf + n;
	n = MAXIO - n;
	if(n > max)
		n = max;
	n = read(f, stop, n);
	if(n <= 0)
		return 0;
	return stop + n;
}

void
ldobj(int f, long c, char *pn)
{
	Prog *p, *t;
	Sym *h[NSYM], *s, *di;
	int v, o, r, skip;
	long ipc;
	uchar *bloc, *bsize, *stop;
	Adr a;

	bsize = buf.xbuf;
	bloc = buf.xbuf;
	di = S;

newloop:
	memset(h, 0, sizeof(h));
	histfrogp = 0;
	version++;
	ipc = pc;
	skip = 0;

loop:
	if(c <= 0)
		goto eof;
	r = bsize - bloc;
	if(r < 100 && r < c) {		/* enough for largest prog */
		bsize = readsome(f, buf.xbuf, bloc, bsize, c);
		if(bsize == 0)
			goto eof;
		bloc = buf.xbuf;
		goto loop;
	}
	o = bloc[0];		/* as */
	if(o <= 0 || o >= ALAST) {
		diag("%s: opcode out of range %d", pn, o);
		print("	probably not a .z file\n");
		errorexit();
	}
	if(o == ANAME) {
		stop = memchr(&bloc[3], 0, bsize-&bloc[3]);
		if(stop == 0){
			bsize = readsome(f, buf.xbuf, bloc, bsize, c);
			if(bsize == 0)
				goto eof;
			bloc = buf.xbuf;
			stop = memchr(&bloc[3], 0, bsize-&bloc[3]);
			if(stop == 0){
				fprint(2, "%s: name too long\n", pn);
				errorexit();
			}
		}
		v = bloc[1];	/* type */
		o = bloc[2];	/* sym */
		bloc += 3;
		c -= 3;

		r = 0;
		if(v == D_STATIC)
			r = version;
		s = lookup((char*)bloc, r);
		c -= &stop[1] - bloc;
		bloc = stop + 1;

		if(debug['W'])
			print("	ANAME	%s\n", s->name);
		h[o] = s;
		if((v == D_EXTERN || v == D_STATIC) && s->type == 0)
			s->type = SXREF;
		if(v == D_FILE) {
			if(s->type != SFILE) {
				histgen++;
				s->type = SFILE;
				s->value = histgen;
			}
			if(histfrogp < MAXHIST) {
				histfrog[histfrogp] = s;
				histfrogp++;
			} else
				collapsefrog(s);
		}
		goto loop;
	}

	if(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);

	p->as = o;
	p->size = bloc[1];
	p->line = bloc[2] | (bloc[3]<<8) | (bloc[4]<<16) | (bloc[5]<<24);
	r = zaddr(bloc+6, &p->from, h) + 6;
	r += zaddr(bloc+r, &p->to, h);
	bloc += r;
	c -= r;

	p->link = P;
	p->pcond = P;

	if(debug['W'])
		print("%P\n", p);

	switch(o) {
	case AHISTORY:
		if(p->to.offset == -1) {
			addlib(pn);
			histfrogp = 0;
			goto loop;
		}
		addhist(p->line, D_FILE);		/* 'z' */
		if(p->to.offset)
			addhist(p->to.offset, D_FILE1);	/* 'Z' */
		histfrogp = 0;
		goto loop;

	case AEND:
		histtoauto();
		if(curtext != P)
			curtext->to.autom = curauto;
		curauto = 0;
		curtext = P;
		if(c)
			goto newloop;
		return;

	case AGLOBL:
	case APM:
		s = p->from.sym;
		if(s == S) {
			diag("GLOBL must have a name\n%P", p);
			errorexit();
		}
		if(s->type == 0 || s->type == SXREF) {
			s->type = SBSS;
			s->value = 0;
		}
		if(s->type != SBSS) {
			diag("redefinition: %s\n%P", s->name, p);
			s->type = SBSS;
			s->value = 0;
		}
		if(p->to.offset > s->value)
			s->value = p->to.offset;
		if(o == APM)
			s->type = SPMBSS;
		break;

	case ADYNT:
		if(p->to.sym == S) {
			diag("DYNT without a sym\n%P", p);
			break;
		}
		di = p->to.sym;
		p->size = 4;
		if(di->type == SXREF) {
			if(debug['z'])
				Bprint(&bso, "%P set to %d\n", p, dtype);
			di->type = SCONST;
			di->value = dtype;
			dtype += 4;
		}
		if(p->from.sym == S)
			break;

		p->from.offset = di->value;
		p->from.sym->type = SDATA;
		if(curtext == P) {
			diag("DYNT not in text: %P", p);
			break;
		}
		p->to.sym = curtext->from.sym;
		p->to.type = D_CONST;
		p->link = datap;
		datap = p;
		break;

	case AINIT:
		if(p->from.sym == S) {
			diag("INIT without a sym\n%P", p);
			break;
		}
		if(di == S) {
			diag("INIT without previous DYNT\n%P", p);
			break;
		}
		p->from.offset = di->value;
		p->from.sym->type = SDATA;
		p->link = datap;
		datap = p;
		break;

	case ADATA:
		p->link = datap;
		datap = p;
		break;

	case AGOK:
		diag("unknown opcode\n%P", p);
		p->pc = pc;
		pc++;
		break;

	case ATEXT:
		if(curtext != P) {
			histtoauto();
			curtext->to.autom = curauto;
			curauto = 0;
		}
		curtext = p;
		autosize = (p->to.offset+1L) & ~1L;
		p->to.offset = autosize;
		autosize += AUTINC;
		s = p->from.sym;
		if(s == S) {
			diag("TEXT must have a name\n%P", p);
			errorexit();
		}
		if(s->type != 0 && s->type != SXREF)
			diag("redefinition: %s\n%P", s->name, p);
		s->type = STEXT;
		s->value = pc;
		if(textp != P) {
			for(t = textp; t->pcond != P; t = t->pcond)
				;
			t->pcond = p;
		} else
			textp = p;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		pc++;
		break;

	case AOUT:
	case ABCLR:
	case ABSET:
		a = p->from;
		p->from = p->to;
		p->to = a;
		goto casedef;

	case ABRBS:
		switch(p->from.offset){
		case 0:	p->as = ABLO; break;
		case 1:	p->as = ABEQ; break;
		case 2:	p->as = ABMI; break;
		case 3:	p->as = ABVS; break;
		case 4:	p->as = ABLT; break;
		case 5:	p->as = ABHS; break;
		case 6:	p->as = ABTS; break;
		case 7:	p->as = ABIE; break;
		default:	diag("bad bit field %d in brbs", p->from.offset);
		}
		p->from.type = D_NONE;
		goto casedef;

	case ABRBC:
		switch(p->from.offset){
		case 0:	p->as = ABSH; break;
		case 1:	p->as = ABNE; break;
		case 2:	p->as = ABPL; break;
		case 3:	p->as = ABVC; break;
		case 4:	p->as = ABGE; break;
		case 5:	p->as = ABHC; break;
		case 6:	p->as = ABTC; break;
		case 7:	p->as = ABID; break;
		default:	diag("bad bit field %d in brbc", p->from.offset);
		}
		p->from.type = D_NONE;
		goto casedef;

	case ASAVE:
	case ARESTORE:
		reguse = 1;
		goto casedef;

	default:
	casedef:
		if(skip)
			nopout(p);
		if(p->to.type == D_BRANCH)
			p->to.offset += ipc;
		lastp->link = p;
		lastp = p;
		p->pc = pc;
		pc++;
		break;
	}
	goto loop;

eof:
	diag("truncated object file: %s", pn);
}

Sym*
lookup(char *symb, int v)
{
	Sym *s;
	char *p;
	long h;
	int c, l;

	h = v;
	for(p=symb; c = *p; p++)
		h = h+h+h + c;
	l = (p - symb) + 1;
	if(h < 0)
		h = ~h;
	h %= NHASH;
	for(s = hash[h]; s != S; s = s->link)
		if(s->version == v)
		if(memcmp(s->name, symb, l) == 0)
			return s;

	while(nhunk < sizeof(Sym))
		gethunk();
	s = (Sym*)hunk;
	nhunk -= sizeof(Sym);
	hunk += sizeof(Sym);

	s->name = malloc(l + 1);
	memmove(s->name, symb, l);

	s->link = hash[h];
	s->type = 0;
	s->version = v;
	s->fnptr = 0;
	s->value = 0;
	s->regused = 0;
	hash[h] = s;
	return s;
}

Sym*
lookup0(char *symb)
{
	Sym *s, *sym;
	char *p;
	long h;
	int l;

	for(p=symb; *p; p++)
		;
	l = (p - symb) + 1;
	sym = S;
	for(h = 0; h < NHASH; h++){
		for(s = hash[h]; s != S; s = s->link){
			if(s->name[0] == symb[0] && memcmp(s->name, symb, l) == 0){
				if(sym != S)
					return S;	/* ambiguous */
				sym = s;
			}
		}
	}
	return sym;
}

Prog*
prg(void)
{
	Prog *p;
	int n;

	n = (sizeof(Prog) + 3) & ~3;
	while(nhunk < n)
		gethunk();

	p = (Prog*)hunk;
	nhunk -= n;
	hunk += n;

	*p = zprg;
	return p;
}

void
gethunk(void)
{
	char *h;

	h = mysbrk((int)NHUNK);
	if(h == (char *)-1) {
		diag("out of memory");
		errorexit();
	}

	hunk = h;
	nhunk = NHUNK;
	tothunk += NHUNK;
}

void
doprof1(void)
{
}

void
doprof2(void)
{
}

void
nuxiinit(void)
{
	int i, c;

	for(i=0; i<4; i++) {
		c = find1(0x04030201L, i+1);
		if(i < 2)
			inuxi2[i] = c;
		if(i < 1)
			inuxi1[i] = c;
		inuxi4[i] = c;
		fnuxi8[i] = c;
		fnuxi8[i+4] = c+4;
	}
	if(debug['v']) {
		Bprint(&bso, "inuxi = ");
		for(i=0; i<1; i++)
			Bprint(&bso, "%d", inuxi1[i]);
		Bprint(&bso, " ");
		for(i=0; i<2; i++)
			Bprint(&bso, "%d", inuxi2[i]);
		Bprint(&bso, " ");
		for(i=0; i<4; i++)
			Bprint(&bso, "%d", inuxi4[i]);
		Bprint(&bso, "\nfnuxi = ");
		for(i=0; i<8; i++)
			Bprint(&bso, "%d", fnuxi8[i]);
		Bprint(&bso, "\n");
	}
	Bflush(&bso);
}

int
find1(long l, int c)
{
	char *p;
	int i;

	p = (char*)&l;
	for(i=0; i<4; i++)
		if(*p++ == c)
			return i;
	return 0;
}

long
ieeedtof(Ieee *ieeep)
{
	int exp;
	long v;

	if(ieeep->h == 0)
		return 0;
	exp = (ieeep->h>>20) & ((1L<<11)-1L);
	exp -= (1L<<10) - 2L;
	v = (ieeep->h & 0xfffffL) << 3;
	v |= (ieeep->l >> 29) & 0x7L;
	if((ieeep->l >> 28) & 1) {
		v++;
		if(v & 0x800000L) {
			v = (v & 0x7fffffL) >> 1;
			exp++;
		}
	}
	if(exp <= -126 || exp >= 130)
		diag("double fp to single fp overflow");
	v |= ((exp + 126) & 0xffL) << 23;
	v |= ieeep->h & 0x80000000L;
	return v;
}

double
ieeedtod(Ieee *ieeep)
{
	Ieee e;
	double fr;
	int exp;

	if(ieeep->h & (1L<<31)) {
		e.h = ieeep->h & ~(1L<<31);
		e.l = ieeep->l;
		return -ieeedtod(&e);
	}
	if(ieeep->l == 0 && ieeep->h == 0)
		return 0;
	fr = ieeep->l & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (ieeep->l>>16) & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (ieeep->h & (1L<<20)-1L) | (1L<<20);
	fr /= 1L<<21;
	exp = (ieeep->h>>20) & ((1L<<11)-1L);
	exp -= (1L<<10) - 2L;
	return ldexp(fr, exp);
}
