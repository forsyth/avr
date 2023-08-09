#include	"l.h"

#define	LPUT(c)\
	{\
		cbp[0] = (c)>>24;\
		cbp[1] = (c)>>16;\
		cbp[2] = (c)>>8;\
		cbp[3] = (c);\
		cbp += 4;\
		cbc -= 4;\
		if(cbc <= 0)\
			cflush();\
	}

#define	CPUT(c)\
	{\
		cbp[0] = (c);\
		cbp++;\
		cbc--;\
		if(cbc <= 0)\
			cflush();\
	}

void	strnput(char*, int);
static void fnptrs(void);
static void ruse(void);
static int stkuse(Prog*, int);
static void readfp(char*);
static int callg(Prog*, int, int, int*, int, int);

long
entryvalue(void)
{
	char *a;
	Sym *s;

	a = INITENTRY;
	if(*a >= '0' && *a <= '9')
		return atolwhex(a);
	s = lookup(a, 0);
	if(s->type == 0)
		return INITTEXT;
	if(s->type != STEXT && s->type != SLEAF)
		diag("entry not text: %s", s->name);
	if(debug['v'])
		Bprint(&bso, "entry = %lux (byte) %lux (word)\n", s->value, s->value/2);
	return s->value/2;
}

/* determine stack usage */
void
asms(int stkpc)
{
	int m1, m2, s;
	Prog *p;
	
	fnptrs();
	for(p = textp; p != P; p = p->pcond){
		p->flag = 0;
		p->forwd = P;
	}
	readfp("FP");
	m1 = 0;
	/* mark callees */
	for(p = textp; p != P; p = p->pcond)
		if(p->flag == 0)
			callg(p, 0, 0, &m1, 0, stkpc);
	m1 = 0;
	m2 = 0;
	/* print out */
	for(p = textp; p != P; p = p->pcond){
		if(p->flag == 0){
			s = callg(p, 0, 0, &m1, 1, stkpc);
			print("%s maxdepth %d\n", p->from.sym->name, s);
			if(s > m2)
				m2 = s;
		}
	}
	if(m1 != m2){
		diag("asms: %d != %d", m1, m2);
		errorexit();
	}
	print("Maxdepth %d\n", m1);
}

/* determine register usage only (SAVE and RESTORE) */
void
asmr(void)
{
	Prog *p;
	long r;
	Optab *o;

	curtext = P;
	r = 0;
	pc = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			if(curtext != P){
				curtext->from.sym->regused = r;
				curtext->flag = 0;
			}
			curtext = p;
			r = 0;
		}
		p->pc = pc;
		o = oplook(p);
		r |= asmout(p, o, 0);
		pc += 2;
	}
	if(curtext != P){
		curtext->from.sym->regused = r;
		curtext->flag = 0;
	}
	ruse();
}

void
asmb(void)
{
	Prog *p;
	long t;
	Optab *o;

	if(debug['v'])
		Bprint(&bso, "%5.2f asm\n", cputime());
	Bflush(&bso);
	seek(cout, HEADR, 0);
	pc = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			autosize = p->to.offset + AUTINC;
		}
		if(p->pc != pc) {
			diag("phase error %lux sb %lux",
				p->pc, pc);
			if(!debug['a'])
				prasm(curp);
			pc = p->pc;
		}
		curp = p;
		o = oplook(p);	/* could probably avoid this call */
		asmout(p, o, 1);
		pc += o->size;
	}
	if(debug['a'])
		Bprint(&bso, "\n");
	Bflush(&bso);
	cflush();

	if(debug['v'])
		Bprint(&bso, "hdr = %lux\n", HEADR);

	curtext = P;
	switch(HEADTYPE) {
	case 0:
		seek(cout, HEADR+textsize, 0);
		break;
	case 1:
	case 2:
	case 5:
	case 6:
		seek(cout, HEADR+textsize, 0);
		break;
	case 4:
		seek(cout, rnd(HEADR+textsize, 4096), 0);
		break;
	}
	for(t = 0; t < datsize; t += sizeof(buf)-100) {
		if(datsize-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100, 0);
		else
			datblk(t, datsize-t, 0);
	}

	symsize = 0;
	lcsize = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);
		switch(HEADTYPE) {
		case 0:
			seek(cout, HEADR+textsize+datsize, 0);
			break;
		case 2:
		case 1:
		case 5:
		case 6:
			seek(cout, HEADR+textsize+datsize, 0);
			break;
		case 4:
			seek(cout, rnd(HEADR+textsize, 4096)+datsize, 0);
			break;
		}
		if(!debug['s'])
			asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f sp\n", cputime());
		Bflush(&bso);
		if(!debug['s'])
			asmlc();
		if(HEADTYPE == 0 || HEADTYPE == 1)	/* round up file length for boot image */
			if((symsize+lcsize) & 1)
				CPUT(0);
		cflush();
	}

	seek(cout, 0L, 0);
	switch(HEADTYPE) {
	case 0:
		lput(0x1030107);		/* magic and sections */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	case 1:
	case 6:
		/* no header */
		break;
	case 2:
		lput(4*11*11+7);
		/* lput(4*25*25+7);	*/	/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	}
	cflush();
	if(debug['v']){
		t = HEADR+textsize+datsize+symsize+lcsize;
		Bprint(&bso, "total size without bss = %lux (%ld)\n", t, t);
	}
}

void
strnput(char *s, int n)
{
	for(; *s; s++){
		CPUT(*s);
		n--;
	}
	for(; n > 0; n--)
		CPUT(0);
}

void
hputl(long l)
{
	cbp[1] = l>>8;
	cbp[0] = l;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
lput(long l)
{
	LPUT(l);
}

void
lputl(long l)
{
	cbp[3] = l>>24;
	cbp[2] = l>>16;
	cbp[1] = l>>8;
	cbp[0] = l;
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
cflush(void)
{
	int n;

	n = sizeof(buf.cbuf) - cbc;
	if(n)
		write(cout, buf.cbuf, n);
	cbp = buf.cbuf;
	cbc = sizeof(buf.cbuf);
}

void
asmsym(void)
{
	Prog *p;
	Auto *a;
	Sym *s;
	int h;

	s = lookup("etext", 0);
	if(s->type == STEXT)
		putsymb(s->name, 'T', s->value, s->version);

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
			case SCONST:
				putsymb(s->name, 'D', s->value, s->version);
				continue;

			case SDATA:
				putsymb(s->name, 'D', s->value+INITDAT, s->version);
				continue;

			case SBSS:
				putsymb(s->name, 'B', s->value+INITDAT, s->version);
				continue;

			case SFILE:
				putsymb(s->name, 'f', s->value, s->version);
				continue;

			case SPMDATA:
			case SPMBSS:
				putsymb(s->name, 'T', s->value, s->version);
				continue;
			}

	for(p=textp; p!=P; p=p->pcond) {
		s = p->from.sym;
		if(s->type != STEXT && s->type != SLEAF)
			continue;

		/* filenames first */
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_FILE)
				putsymb(a->asym->name, 'z', a->aoffset, 0);
			else
			if(a->type == D_FILE1)
				putsymb(a->asym->name, 'Z', a->aoffset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+4, 0);
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->asym->name, 'a', -a->aoffset, 0);
			else
			if(a->type == D_PARAM)
				putsymb(a->asym->name, 'p', a->aoffset, 0);
	}
	if(debug['v'] || debug['n'])
		Bprint(&bso, "symsize = %lux\n", symsize);
	Bflush(&bso);
}

void
putsymb(char *s, int t, long v, int ver)
{
	int i, f;

	if(t == 'f')
		s++;
	LPUT(v);
	if(ver)
		t += 'a' - 'A';
	CPUT(t+0x80);			/* 0x80 is variable length */

	if(t == 'Z' || t == 'z') {
		CPUT(s[0]);
		for(i=1; s[i] != 0 || s[i+1] != 0; i += 2) {
			CPUT(s[i]);
			CPUT(s[i+1]);
		}
		CPUT(0);
		CPUT(0);
		i++;
	}
	else {
		for(i=0; s[i]; i++)
			CPUT(s[i]);
		CPUT(0);
	}
	symsize += 4 + 1 + i + 1;

	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			Bprint(&bso, "%c %.8lux ", t, v);
			for(i=1; s[i] != 0 || s[i+1] != 0; i+=2) {
				f = ((s[i]&0xff) << 8) | (s[i+1]&0xff);
				Bprint(&bso, "/%x", f);
			}
			Bprint(&bso, "\n");
			return;
		}
		if(ver)
			Bprint(&bso, "%c %.8lux %s<%d>\n", t, v, s, ver);
		else
			Bprint(&bso, "%c %.8lux %s\n", t, v, s);
	}
}

#define	MINLC	4
void
asmlc(void)
{
	long oldpc, oldlc;
	Prog *p;
	long v, s;

	oldpc = INITTEXT;
	oldlc = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->line == oldlc || p->as == ATEXT || p->as == ANOOP) {
			if(p->as == ATEXT)
				curtext = p;
			if(debug['L'])
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			continue;
		}
		if(debug['L'])
			Bprint(&bso, "\t\t%6ld", lcsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			s = 127;
			if(v < 127)
				s = v;
			CPUT(s+128);	/* 129-255 +pc */
			if(debug['L'])
				Bprint(&bso, " pc+%ld*%d(%ld)", s, MINLC, s+128);
			v -= s;
			lcsize++;
		}
		s = p->line - oldlc;
		oldlc = p->line;
		oldpc = p->pc + MINLC;
		if(s > 64 || s < -64) {
			CPUT(0);	/* 0 vv +lc */
			CPUT(s>>24);
			CPUT(s>>16);
			CPUT(s>>8);
			CPUT(s);
			if(debug['L']) {
				if(s > 0)
					Bprint(&bso, " lc+%ld(%d,%ld)\n",
						s, 0, s);
				else
					Bprint(&bso, " lc%ld(%d,%ld)\n",
						s, 0, s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
			lcsize += 5;
			continue;
		}
		if(s > 0) {
			CPUT(0+s);	/* 1-64 +lc */
			if(debug['L']) {
				Bprint(&bso, " lc+%ld(%ld)\n", s, 0+s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		} else {
			CPUT(64-s);	/* 65-128 -lc */
			if(debug['L']) {
				Bprint(&bso, " lc%ld(%ld)\n", s, 64-s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		}
		lcsize++;
	}
	while(lcsize & 1) {
		s = 129;
		CPUT(s);
		lcsize++;
	}
	if(debug['v'] || debug['L'])
		Bprint(&bso, "lcsize = %lux\n", lcsize);
	Bflush(&bso);
}

void
pmdata(Prog *p)
{
	long b, t, pc, epc;

	cflush();
	b = sizeof(buf)-100;
	pc = p->pc;
	epc = p->pc + pmdatsize;
	/* data */
	for(t = pc; t < epc; t += b){
		if(epc-t > b)
			datblk(t, b, 1);
		else
			datblk(t, epc-t, 1);
	}
	/* bss */
	memset(buf.dbuf, 0, sizeof(buf));
	epc = pmbsssize;
	for(t = 0; t < epc; t += b){
		if(epc-t > b)
			write(cout, buf.dbuf, b);
		else
			write(cout, buf.dbuf, epc-t);
	}
	if(pmdatsize+pmbsssize != p->from.offset)
		diag("bad pmdata sizes");
}

void
datblk(long s, long n, int pm)
{
	Prog *p;
	char *cast;
	long l, fl, j, d;
	int i, c;

	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		if(pm != (p->from.sym->type == SPMDATA))
			continue;
		curp = p;
		l = p->from.sym->value + p->from.offset - s;
		c = p->size;
		i = 0;
		if(l < 0) {
			if(l+c <= 0)
				continue;
			while(l < 0) {
				l++;
				i++;
			}
		}
		if(l >= n)
			continue;
		if(p->as != AINIT && p->as != ADYNT) {
			for(j=l+(c-i)-1; j>=l; j--)
				if(buf.dbuf[j]) {
					print("%P\n", p);
					diag("multiple initialization");
					break;
				}
		}
		switch(p->to.type) {
		default:
			diag("unknown mode in initialization\n%P", p);
			break;

		case D_FCONST:
			switch(c) {
			default:
			case 4:
				fl = ieeedtof(&p->to.ieee);
				cast = (char*)&fl;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i+4]];
					l++;
				}
				break;
			case 8:
				cast = (char*)&p->to.ieee;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i]];
					l++;
				}
				break;
			}
			break;

		case D_SCONST:
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.sval[i];
				l++;
			}
			break;

		case D_CONST:
			d = p->to.offset;
			if(p->to.sym) {
				switch(p->to.sym->type){
				case STEXT:
				case SLEAF:
					d = (d+p->to.sym->value)/2;
					break;
				case SPMDATA:
				case SPMBSS:
					d += p->to.sym->value;
					break;
				case SDATA:
				case SBSS:
					d += p->to.sym->value + INITDAT;
					break;
				}
			}
			cast = (char*)&d;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P", c, i, curp);
				break;
			case 1:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi1[i]];
					l++;
				}
				break;
			case 2:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi2[i]];
					l++;
				}
				break;
			case 4:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi4[i]];
					l++;
				}
				break;
			}
			break;
		}
	}
	write(cout, buf.dbuf, n);
}

static int
ru(Prog *p)
{
	int r;
	Prog *t;

	t = p;
	if(t->from.sym == S)
		return 0;
	r = t->from.sym->regused;
	if(t->flag)
		return r;
	t->flag = 1;
	for(p = p->link ; p != P; p = p->link){
		switch(p->as){
		case ATEXT:
			t->flag = 0;
			return r;
		case ACALL:
		case AVECTOR:
			if(p->to.type == D_BRANCH)
				r |= ru(p->pcond);
			else
				r |= ~0;
			break;
		}
	}
	t->flag = 0;
	return r;
}

static void
ruse(void)
{
	Prog *p;
	
	for(p = textp; p != P; p = p->pcond){
		p->from.sym->regused |= ru(p);
		p->flag = 1;
	}	
}

static Prog**
addg(Prog **c, int *n, Prog *p)
{
	int i, nc;
	Prog **c0;

	nc = *n;
	for(i = 0; i < nc; i++)
		if(c[i] == p)
			return c;
	if((nc&7) == 0){
		c0 = malloc((nc+8)*sizeof(Prog*));
		memmove(c0, c, nc*sizeof(Prog*));
		free(c);
		c = c0;
	}
	c[nc++] = p;
	*n = nc;
	return c;
}

static int
callg(Prog *p, int ss, int depth, int *max, int pass, int stkpc)
{
	int i, nc, m, s, su;
	Prog *q, **c;

	su = stkuse(p, stkpc);
	if(p->flag&1)
		return su;
	ss += su;
	if(ss > *max)
		*max = ss;
	if(pass){
		for(i = 0; i < depth; i++)
			print("        ");
		print("%s %d", p->from.sym->name, ss);
	}
	m = 0;
	c = nil;
	nc = 0;
	p->flag |= 1;
	for(q = p->forwd; q != P; q = q->link){
		if(q->pcond->as == ATEXT){
			c = addg(c, &nc, q->pcond);
			if(pass && nc == 1)
				print("\n");
			s = callg(q->pcond, ss, depth+1, max, pass, stkpc);
			if(s > m)
				m = s;
		}
	}
	for(q = p->link ; q != P; q = q->link){
		switch(q->as){
		case ATEXT:
			p->flag &= ~1;
			if(depth > 0)
				p->flag = 2;
			if(pass && nc == 0)
				print("L\n");
			free(c);
			return m+su;
		case ACALL:
		case AVECTOR:
			if(q->to.type == D_BRANCH && q->pcond->as == ATEXT){
				c = addg(c, &nc, q->pcond);
				if(pass && nc == 1)
					print("\n");
				s = callg(q->pcond, ss, depth+1, max, pass, stkpc);
				if(s > m)
					m = s;
			}
			break;
		}
	}
	p->flag &= ~1;
	if(depth > 0)
		p->flag = 2;
	if(pass && nc == 0)
		print("L\n");
	free(c);
	return m+su;
}

static int
stkuse(Prog *p, int stkpc)
{
	int n;

	if(stkpc)
		return 2;
	n = p->to.offset+AUTINC;
	if(n < 0)
		return 0;
	return n;
}

static int
isfn(Adr* a)
{
	if(a->type == D_OREG)
	if(a->name == D_EXTERN || a->name == D_STATIC)
	if(a->sym != S)
	if(a->sym->type == STEXT || a->sym->type == SLEAF)
		return 1;
	return 0;
}

static int
isfnptr(Adr* a)
{
	if(a->type == D_CONST)
	if(a->name == D_EXTERN || a->name == D_STATIC)
	if(a->sym != S)
	if(a->sym->type == STEXT || a->sym->type == SLEAF)
		return 1;
	return 0;
}

static void
fnptrs(void)
{
	Prog *p;

	for(p = firstp; p != P; p = p->link){
		if(p->as != ATEXT && isfnptr(&p->from)){
			if(p->from.sym->fnptr == 0)
				print("fp: %s\n", p->from.sym->name);
			p->from.sym->fnptr = 1;
		}
		if(p->as == AVECTOR && isfn(&p->to)){
			if(p->to.sym->fnptr == 0)
				print("fp: vector/%s\n", p->to.sym->name);
			p->to.sym->fnptr = 1;
		}
	}
	for(p = datap; p != P; p = p->link){
		if(isfnptr(&p->to)){
			if(p->to.sym->fnptr == 0)
				print("fp: %s/%s\n", p->from.sym->name, p->to.sym->name);
			p->to.sym->fnptr = 1;
		}
	}
}

static Prog*
findp(char *s)
{
	Sym *sym;
	Prog *p;

	sym = lookup0(s);
	if(sym == S){
		diag("%s undefined/ambiguous", s);
		errorexit();
	}
	if(sym->type != STEXT && sym->type != SLEAF){
		diag("%s is not a function", s);
		errorexit();
	}
	for(p = textp; p != P; p = p->pcond)
		if(p->from.sym == sym)
			return p;
	diag("%s has no code\n", s);
	errorexit();
	return P;
}

static void
readfp(char *f)
{
	int n;
	Biobuf *b;
	char *l, buf[64], *fields[3];
	Prog *p, *q, *r;

	b = Bopen(f, OREAD);
	if(b == nil)
		return;
	while((l = Brdline(b, '\n')) != nil){
		if(*l == '#')
			continue;
		n = Blinelen(b);
		if(n >= sizeof(buf)){
			diag("%s: line too long", f);
			errorexit();
		}
		memmove(buf, l, n);
		buf[n-1] = '\0';
		n = getfields(buf, fields, nelem(fields), 1, " \t\r\n");
		if(n != 2){
			diag("%s: bad format", f);
			errorexit();
		}
		p = findp(fields[0]);
		q = findp(fields[1]);
		/* fake up a call */
		r = prg();
		r->as = ACALL;
		r->to.type = D_BRANCH;
		r->pcond = q;
		r->link = p->forwd;
		p->forwd = r;
	}
	Bterm(b);
}

	