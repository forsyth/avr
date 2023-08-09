#include	"l.h"

void
dodata(void)
{
	int i, t;
	Sym *s;
	Prog *p;
	long orig, v;
	// long tot=0;

	if(debug['v'])
		Bprint(&bso, "%5.2f dodata\n", cputime());
	Bflush(&bso);
	for(p = datap; p != P; p = p->link) {
		s = p->from.sym;
		if(p->as == ADYNT || p->as == AINIT)
			s->value = dtype;
		if(s->type == SBSS)
			s->type = SDATA;
		if(s->type == SPMBSS)
			s->type = SPMDATA;
		if(s->type != SDATA && s->type != SPMDATA)
			diag("initialize non-data (%d): %s\n%P",
				s->type, s->name, p);
		v = p->from.offset + p->size;
		if(v > s->value)
			diag("initialize bounds (%ld): %s\n%P",
				s->value, s->name, p);
	}

	/*
	 * pass 1
	 *	assign 'small' variables to data segment
	 *	(rational is that data segment is more easily
	 *	 addressed through offset on REGSB)
	 */
	orig = 0;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA && t != SBSS)
			continue;
		v = s->value;
		if(v == 0) {
			diag("%s: no size", s->name);
			v = 1;
		}
		while(v & 1)
			v++;
		s->value = v;
		if(v > MINSIZ)
			continue;
// print("d %s	%ld\n", s->name, v); tot += v;
		s->value = orig;
		orig += v;
		s->type = SDATA1;
	}

	/*
	 * pass 2
	 *	assign 'data' variables to data segment
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA) {
			if(t == SDATA1)
				s->type = SDATA;
			continue;
		}
		v = s->value;
// print("d %s	%ld\n", s->name, v); tot += v;
		s->value = orig;
		orig += v;
	}

	while(orig & 1)
		orig++;
	datsize = orig;

	/*
	 * pass 3
	 *	everything else to bss segment
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SBSS)
			continue;
		v = s->value;
// print("b %s	%ld\n", s->name, v); tot += v;
		s->value = orig;
		orig += v;
	}
	while(orig & 1)
		orig++;
	bsssize = orig-datsize;

// print("total = %ld\n", tot);

	// xdefine("setSB", SDATA, 0L+BIG);
	xdefine("bdata", SDATA, 0L);
	xdefine("edata", SDATA, datsize);
	xdefine("end", SBSS, datsize+bsssize);
	xdefine("etext", STEXT, 0L);
	xdefine("_iomem", SDATA, -(64+160));
	if(debug['v'])
		Bprint(&bso, "dsize = %lux bsize = %lux\n", datsize, bsssize);

	/*
	 * pass 4
	 *	program memory data
	 */
	orig = 0;
	for(i = 0; i < NHASH; i++)
	for(s = hash[i]; s != S; s = s->link){
		if(s->type != SPMDATA)
			continue;
		v = s->value;
		if(v & 1)
			v++;
		s->value = orig;
		orig += v;
	}
	pmdatsize = orig;

	/*
	 * pass 5
	 *	program memory bss
	 */
	for(i = 0; i < NHASH; i++)
	for(s = hash[i]; s != S; s = s->link){
		if(s->type != SPMBSS)
			continue;
		v = s->value;
		if(v & 1)
			v++;
		s->value = orig;
		orig += v;
	}
	pmbsssize = orig-pmdatsize;	

}

void
undef(void)
{
	int i;
	Sym *s;

	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link)
		if(s->type == SXREF)
			diag("%s: not defined", s->name);
}

int
relne(int a)
{
	switch(a){
	case ABGE:	return ABGT;
	case ABLE:	return ABLT;
	case ABSH:	return ABHI;
	case ABSL:	return ABLO;
	}
	return a;
}

int
relu(int a)
{
	switch(a){
	case ABGT:	return ABHI;
	case ABLT:	return ABLO;
	case ABGE:	return ABSH;
	case ABLE:	return ABSL;
	}
	return a;
}

int
relop(int a)
{
	switch(a){
	case ABGT:	return ABGE;
	case ABLE:	return ABLT;
	case ABHI:	return ABSH;
	case ABSL:	return ABLO;
	default:
		diag("bad op in relop");
		break;
	}
	return 0;
}

int
relinv(int a)
{

	switch(a) {
	case ABEQ:	return ABNE;
	case ABNE:	return ABEQ;

	case ABGE:	return ABLT;
	case ABLT:	return ABGE;

	case ABGT:	return ABLE;
	case ABLE:	return ABGT;

	case ABLO:	return ABSH;
	case ABSH:	return ABLO;

	case ABHI:	return ABSL;
	case ABSL:	return ABHI;

	case ABPL:	return ABMI;
	case ABMI:	return ABPL;

	case ABCC:	return ABCS;
	case ABCS:	return ABCC;

	case ABHC:	return ABHS;
	case ABHS:	return ABHC;

	case ABTC:	return ABTS;
	case ABTS:	return ABTC;

	case ABVC:	return ABVS;
	case ABVS:	return ABVC;

	case ABID:	return ABIE;
	case ABIE:		return ABID;

	case ABRBS:	return ABRBC;
	case ABRBC:	return ABRBS;

	default:
		// diag("cannot relinv %A", a);
		return 0;
	}
}

static int
isskip(Prog *p)
{
	switch(p->as){
	case ASBRC:
	case ASBRS:
	case ASBIC:
	case ASBIS:
		return 1;
	}
	return 0;
}
	
void
follow(void)
{

	if(debug['v'])
		Bprint(&bso, "%5.2f follow\n", cputime());
	Bflush(&bso);

	firstp = prg();
	lastp = firstp;

	xfol(textp);

	firstp = firstp->link;
	lastp->link = P;
}

void
xfol(Prog *p)
{
	Prog *q, *r;
	int a, b, i;

loop:
	if(p == P)
		return;
	a = p->as;
	if(a == ATEXT)
		curtext = p;
	if(a == ABR) {
		q = p->pcond;
		if(q != P) {
			p->mark |= FOLL;
			p = q;
			if(!(p->mark & FOLL))
				goto loop;
		}
	}
	if(p->mark & FOLL) {
		for(i=0,q=p; i<4; i++,q=q->link) {
			if(q == lastp)
				break;
			b = 0;		/* set */
			a = q->as;
			if(a == ANOOP) {
				i--;
				continue;
			}
			if(a == ABR || a == ARET || a == ARETI)
				goto copy;
			if(!q->pcond || (q->pcond->mark&FOLL))
				continue;
			b = relinv(a);
			if(!b)
				continue;
		copy:
			for(;;) {
				r = prg();
				*r = *p;
				if(p->pcond == p)
					r->pcond = r;
				if(!(r->mark&FOLL))
					print("cant happen 1\n");
				r->mark |= FOLL;
				if(p != q) {
					p = p->link;
					lastp->link = r;
					lastp = r;
					continue;
				}
				lastp->link = r;
				lastp = r;
				if(a == ABR || a == ARET || a == ARETI)
					return;
				r->as = b;
				r->pcond = p->link;
				r->link = p->pcond;
				if(!(r->link->mark&FOLL))
					xfol(r->link);
				if(!(r->pcond->mark&FOLL))
					print("cant happen 2\n");
				return;
			}
		}

		a = ABR;
		q = prg();
		q->as = a;
		q->line = p->line;
		q->to.type = D_BRANCH;
		q->to.offset = p->pc;
		q->pcond = p;
		p = q;
	}
	p->mark |= FOLL;
	lastp->link = p;
	lastp = p;
	if(a == ABR || a == ARET || a == ARETI){
		return;
	}
	if(isskip(p)){
		p = p->link;
		if(p == P)
			return;
		p->mark |= FOLL;
		lastp->link = p;
		lastp = p;
		p = p->link;
		if(p == P)
			return;
		p->mark |= FOLL;
		lastp->link = p;
		lastp = p;
	}
	if(p->pcond != P)
	if(a != ACALL && p->link != P) {
		xfol(p->link);
		p = p->pcond;
		if(p == P || (p->mark&FOLL))
			return;
		goto loop;
	}
	p = p->link;
	goto loop;
}

void
patch(void)
{
	long c, vexit;
	Prog *p, *q;
	Sym *s;
	int a;

	if(debug['v'])
		Bprint(&bso, "%5.2f patch\n", cputime());
	Bflush(&bso);
	mkfwd();
	s = lookup("exit", 0);
	vexit = s->value;
	for(p = firstp; p != P; p = p->link) {
		a = p->as;
		if(a == ATEXT)
			curtext = p;
		if((a == ACALL || a == ARET || a == ARETI) && p->to.sym != S) {
			s = p->to.sym;
			if(s->type != STEXT) {
				diag("undefined: %s\n%P", s->name, p);
				s->type = STEXT;
				s->value = vexit;
			}
			p->to.offset = s->value;
			p->to.type = D_BRANCH;
		}
		if(p->to.type != D_BRANCH)
			continue;
		c = p->to.offset;
		for(q = firstp; q != P;) {
			if(q->forwd != P)
			if(c >= q->forwd->pc) {
				q = q->forwd;
				continue;
			}
			if(c == q->pc)
				break;
			q = q->link;
		}
		if(q == P) {
			diag("branch out of range %ld\n%P", c, p);
			p->to.type = D_NONE;
		}
		p->pcond = q;
	}

	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		if(p->pcond != P) {
			p->pcond = brloop(p->pcond);
			if(p->pcond != P)
			if(p->to.type == D_BRANCH)
				p->to.offset = p->pcond->pc;
		}
	}
}

#define	LOG	5
void
mkfwd(void)
{
	Prog *p;
	long dwn[LOG], cnt[LOG], i;
	Prog *lst[LOG];

	for(i=0; i<LOG; i++) {
		if(i == 0)
			cnt[i] = 1; else
			cnt[i] = LOG * cnt[i-1];
		dwn[i] = 1;
		lst[i] = P;
	}
	i = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		i--;
		if(i < 0)
			i = LOG-1;
		p->forwd = P;
		dwn[i]--;
		if(dwn[i] <= 0) {
			dwn[i] = cnt[i];
			if(lst[i] != P)
				lst[i]->forwd = p;
			lst[i] = p;
		}
	}
}

Prog*
brloop(Prog *p)
{
	Prog *q;
	int c;

	for(c=0; p!=P;) {
		if(p->as != ABR)
			return p;
		q = p->pcond;
		if(q <= p) {	/* ????? */
			c++;
			if(q == p || c > 5000)
				break;
		}
		p = q;
	}
	return p;	/* return P; */
}

long
atolwhex(char *s)
{
	long n;
	int f;

	n = 0;
	f = 0;
	while(*s == ' ' || *s == '\t')
		s++;
	if(*s == '-' || *s == '+') {
		if(*s++ == '-')
			f = 1;
		while(*s == ' ' || *s == '\t')
			s++;
	}
	if(s[0]=='0' && s[1]){
		if(s[1]=='x' || s[1]=='X'){
			s += 2;
			for(;;){
				if(*s >= '0' && *s <= '9')
					n = n*16 + *s++ - '0';
				else if(*s >= 'a' && *s <= 'f')
					n = n*16 + *s++ - 'a' + 10;
				else if(*s >= 'A' && *s <= 'F')
					n = n*16 + *s++ - 'A' + 10;
				else
					break;
			}
		} else
			while(*s >= '0' && *s <= '7')
				n = n*8 + *s++ - '0';
	} else
		while(*s >= '0' && *s <= '9')
			n = n*10 + *s++ - '0';
	if(f)
		n = -n;
	return n;
}

long
rnd(long v, long r)
{
	long c;

	if(r <= 0)
		return v;
	v += r - 1;
	c = v % r;
	if(c < 0)
		c += r;
	v -= c;
	return v;
}
