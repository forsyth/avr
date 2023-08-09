#include	"l.h"

static void
pmvalues(int v)
{
	int i;
	Sym *s;

	for(i = 0; i < NHASH; i++)
	for(s = hash[i]; s != S; s = s->link){
		if(s->type != SPMDATA && s->type != SPMBSS)
			continue;
		s->value += v;
	}
}

void
span(void)
{
	Prog *p, *q, *lp, *midp;
	Sym *setext;
	Optab *o;
	int m, again, passes;
	long c, lastc, midc;

	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link){
		p->pc = c;
		c += 2;
	}
	midc = (c-INITTEXT)/2 + INITTEXT;
	midp = P;
	for(p = textp; p != P; p = q){
		q = p->pcond;
		if(q == P || q->pc >= midc){
			if(p->pc >= midc || q == P || midc-p->pc <= q->pc-midc)
				midp = p;
			else
				midp = q;
			break;
		}
	}
	c = INITTEXT;
	lp = P;
	for(p = firstp; p != P; p = p->link) {
		p->pc = c;
		o = oplook(p);
		m = o->size;
		if(m == 0) {
			if(p->as == ATEXT) {
				if(p == midp){	/* program data and bss goes here */
					if(lp == P){
						firstp = q = prg();
						q->link = p;
					}
					else
						q = newprg(lp);
					q->as = ADATA;
					con(&q->from, pmdatsize+pmbsssize);
					q->to.type = D_NONE;
					q->size = q->from.offset;
					q->pc = c;
					p->pc = c+q->size;
					o = oplook(q);
					o->size = q->size;	/* only one ADATA in text */
					pmvalues(c-0);
					c += q->size;
				}
				curtext = p;
				autosize = p->to.offset + AUTINC;
				if(p->from.sym != S)
					p->from.sym->value = c;
				lp = p;
				continue;
			}
			if(p->as != ANOOP && p->as != ASAVE && p->as != ARESTORE)
				diag("zero-width instruction\n%P", p);
			lp = p;
			continue;
		}
		c += m;
		lp = p;
	}
	passes = 0;
	lastc = 0;
loop:
	passes++;
	if(passes > 100){
		diag("span looping");
		errorexit();
	}
	again = 0;
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->pc != c)
			again = 1;
		if(p->as == ADATA && c != p->pc)
			pmvalues(c-p->pc);
		p->pc = c;
		o = oplook(p);
		m = o->size;
		if(m == 0) {
			if(p->as == ATEXT) {
				curtext = p;
				autosize = p->to.offset + AUTINC;
				if(p->from.sym != S)
					p->from.sym->value = c;
				continue;
			}
		}
		c += m;
	}
	if(c != lastc || again){
		lastc = c;
		goto loop;
	}

	c = rnd(c, 4);

	setext = lookup("etext", 0);
	if(setext != S) {
		setext->value = c;
		textsize = c - INITTEXT;
	}
	if(INITRND)
		INITDAT = rnd(c, INITRND);
	if(debug['v'])
		Bprint(&bso, "tsize = %lux\n", textsize);
	Bflush(&bso);
}
		
void
xdefine(char *p, int t, long v)
{
	Sym *s;

	s = lookup(p, 0);
	if(s->type == 0 || s->type == SXREF) {
		s->type = t;
		s->value = v;
	}
}

long
regoff(Adr *a, Prog *p)
{

	instoffset = 0;
	aclass(a, p);
	return instoffset;
}

int
aclass(Adr *a, Prog *p)
{
	Sym *s;
	int t;
	long v;

	switch(a->type) {
	case D_NONE:
		return C_NONE;
	case D_REG:
		if(a->reg >= REGZ)
			return C_ZREG;
		if(a->reg >= REGW)
			return C_PREG;
		if(a->reg >= 16)
			return C_HREG;
		return C_REG;
	case D_FREG:
		return C_FREG;
	case D_SREG:
		return C_SREG;
	case D_PREREG:
		return C_PREREG;
	case D_POSTREG:
		if(a->reg == REGZ)
			return C_POSTZREG;
		return C_POSTREG;
	case D_OREG:
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			if(s == S)
				break;
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
			}
			instoffset = s->value + a->offset;
			if(t == SDATA || t == SBSS)
				instoffset += INITDAT;
			if(instoffset >= 0 && instoffset < 65536)
				return C_SEXT;
			return C_LEXT;
		case D_AUTO:
			instoffset = autosize + a->offset + AUTOFF;
			if(instoffset >= 0 && instoffset < 64)
				return C_SAUTO;
			return C_LAUTO;
		case D_PARAM:
			instoffset = autosize + a->offset + PAROFF;
			if(instoffset >= 0 && instoffset < 64)
				return C_SAUTO;
			return C_LAUTO;
		case D_NONE:
			instoffset = a->offset;
			if(instoffset == 0){
				if(a->reg >= REGZ)
					return C_ZOZREG;
				if(a->reg >= REGX)
					return C_ZOPREG;
				return C_ZOREG;
			}
			if(instoffset >= 0 && instoffset < 64){
				if(a->reg >= REGY)
					return C_SOPREG;
				return C_SOREG;
			}
			return C_LOREG;
		}
		return C_GOK;
	case D_CONST:
		switch(a->name) {
		case D_NONE:
			instoffset = a->offset;
		consize:
			if(instoffset >= 0) {
				if(instoffset >= 0){
					if(instoffset < 8)
						return C_BCON;
					if(instoffset < 32)
						return C_ICON;
					if(instoffset < 64)
						return C_QCON;
					if(instoffset < 256)
						return C_KCON;
				}
				return C_LCON;
			}
			return C_LCON;
		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			if(s == S)
				break;
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
			}
			instoffset = s->value + a->offset;
			if(t == STEXT || t == SLEAF)
				instoffset /= 2;
			else if(t == SDATA || t == SBSS)
				instoffset += INITDAT;
			else if(t == SCONST)
				goto consize;
			return C_LCON;
		case D_AUTO:
			instoffset = autosize + a->offset + AUTOFF;
			if(instoffset >= 0 && instoffset < 64)
				return C_SACON;
			return C_LACON;
		case D_PARAM:
			instoffset = autosize + a->offset + PAROFF;
			if(instoffset >= 0 && instoffset < 64)
				return C_SACON;
			return C_LACON;
		}
		return C_GOK;
	case D_BRANCH:
		v = (p->pcond->pc - p->pc)/2 - 1;
		if(v >= -63 && v <= 63)
			return C_BBRA;
		if(v >= -64 && v <= 63)
			return C_CBRA;
		if(v >= -2048 && v <= 2047)
			return C_SBRA;
		return C_LBRA;
	}
	return C_GOK;
}

Optab*
oplook0(Prog *p)
{
	int a1, a2, r, nocache;
	char *c1, *c2;
	Optab *o, *e;

	nocache = 1 || p->to.type == D_BRANCH;
	a1 = p->optab;
	if(!nocache && a1)
		return optab+(a1-1);
	a1 = p->from.class;
	if(nocache || a1 == 0) {
		a1 = aclass(&p->from, p) + 1;
		p->from.class = a1;
	}
	a1--;
	a2 = p->to.class;
	if(nocache || a2 == 0) {
		a2 = aclass(&p->to, p) + 1;
		p->to.class = a2;
	}
	a2--;
	r = p->as;
	o = oprange[r].start;
	if(o == 0)
		o = oprange[r].stop; /* just generate an error */
	e = oprange[r].stop;
	c1 = xcmp[a1];
	c2 = xcmp[a2];
	for(; o<e; o++){
		if(c1[o->a1])
		if(c2[o->a2]) {
			p->optab = (o-optab)+1;
			return o;
		}
	}
	diag("illegal combination %A %R %R",
		p->as, a1, a2);
	if(1||!debug['a'])
		prasm(p);
	if(o == 0)
		errorexit();
	return o;
}

Optab*
oplook(Prog *p)
{
	Optab *o;

	o = oplook0(p);
	if(p->as == ASAVE || p->as == ARESTORE)
		o->size = isize(p);
	return o;
}
	
int
cmp(int a, int b)
{

	if(a == b)
		return 1;
	switch(a) {
	case C_REG:
		if(b == C_ZREG || b == C_PREG || b == C_HREG)
			return 1;
		break;
	case C_HREG:
		if(b == C_ZREG || b == C_PREG)
			return 1;
		break;
	case C_PREG:
		if(b == C_ZREG)
			return 1;
		break;
	case C_LCON:
		if(b == C_BCON || b == C_ICON || b == C_QCON || b == C_KCON)
			return 1;
		break;
	case C_KCON:
		if(b == C_BCON || b == C_ICON || b == C_QCON)
			return 1;
		break;
	case C_QCON:
		if(b == C_BCON || b == C_ICON)
			return 1;
		break;
	case C_ICON:
		if(b == C_BCON)
			return 1;
		break;
	case C_LACON:
		if(b == C_SACON)
			return 1;
		break;
	case C_LECON:
		if(b == C_SECON)
			return 1;
		break;
	case C_LBRA:
		if(b == C_BBRA || b == C_CBRA || b == C_SBRA)
			return 1;
		break;
	case C_SBRA:
		if(b == C_BBRA || b == C_CBRA)
			return 1;
		break;
	case C_CBRA:
		if(b == C_BBRA)
			return 1;
		break;
	case C_LEXT:
		if(b == C_SEXT)
			return 1;
		break;
	case C_LAUTO:
		if(b == C_SAUTO)
			return 1;
		break;
	case C_LOREG:
		if(b == C_ZOZREG || b == C_ZOPREG || b == C_ZOREG || b == C_SOPREG || b == C_SOREG)
			return 1;
		break;
	case C_SOREG:
		if(b == C_ZOZREG || b == C_ZOPREG || b == C_ZOREG || b == C_SOPREG)
			return 1;
		break;
	case C_ZOREG:
		if(b == C_ZOZREG || b == C_ZOPREG)
			return 1;
		break;
	case C_SOPREG:
		if(b == C_ZOZREG || b == C_ZOPREG)
			return 1;
		break;
	case C_ZOPREG:
		if(b == C_ZOZREG)
			return 1;
		break;
	case C_POSTREG:
		if(b == C_POSTZREG)
			return 1;
		break;
	case C_ANY:
		return 1;
	}
	return 0;
}

int
ocmp(void *a1, void *a2)
{
	Optab *p1, *p2;
	int n;

	p1 = a1;
	p2 = a2;
	n = p1->as - p2->as;
	if(n)
		return n;
	n = p1->vers - p2->vers;
	if(n)
		return n;
	n = p1->a1 - p2->a1;
	if(n)
		return n;
	n = p1->a2 - p2->a2;
	if(n)
		return n;
	return 0;
}

void
buildop(void)
{
	int i, n, r;

	for(i=0; i<C_NCLASS; i++)
		for(n=0; n<C_NCLASS; n++)
			xcmp[i][n] = cmp(n, i);
	for(n=0; optab[n].as != AXXX; n++)
		if(optab[n].vers != V0 && optab[n].vers != zversion)
			optab[n].as = AXXX;
	qsort(optab, n, sizeof(optab[0]), ocmp);
	for(i=0; i<n; i++) {
		r = optab[i].as;
		oprange[r].start = optab+i;
		while(optab[i].as == r)
			i++;
		oprange[r].stop = optab+i;
		i--;
		
		switch(r)
		{
		default:
			diag("unknown op in build: %A", r);
			errorexit();
		case AXXX:
			break;
		case AADDB:
			oprange[AADDBC] = oprange[r];
			break;
		case AADDW:
			break;
		case AADDL:
			break;
		case ASUBB:
			oprange[ASUBBC] = oprange[r];
			break;
		case ASUBW:
			break;
		case ASUBL:
			break;
		case AMULB:
			oprange[AMULSUB] = oprange[r];
			oprange[AFMULSB] = oprange[r];
			oprange[AFMULUB] = oprange[r];
			oprange[AFMULSUB] = oprange[r];
			break;
		case AMULUB:
			break;
		case AANDB:
			oprange[AORB] = oprange[r];
			break;
		case AANDW:
			oprange[AORW] = oprange[r];
			break;
		case AANDL:
			oprange[AORL] = oprange[r];
			break;
		case AASRB:
			oprange[ALSLB] = oprange[r];
			oprange[ALSRB] = oprange[r];
			oprange[AROLB] = oprange[r];
			oprange[ARORB] = oprange[r];
			break;
		case ALSLW:
			oprange[AROLW] = oprange[r];
			break;
		case ALSRW:
			oprange[AASRW] = oprange[r];
			oprange[ARORW] = oprange[r];
			break;
		case ALSLL:
			oprange[AROLL] = oprange[r];
			break;
		case ALSRL:
			oprange[AASRL] = oprange[r];
			oprange[ARORL] = oprange[r];
			break;
		case ABREAK:
			oprange[ANOP] = oprange[r];	/* real nop */
			oprange[ASLEEP] = oprange[r];
			oprange[AWDR] = oprange[r];
			break;
		case ARET:
			oprange[ARETI] = oprange[r];
			break;
		case AMOVBW:
			oprange[AMOVBZW] = oprange[r];
			break;
		case AMOVBL:
			oprange[AMOVBZL] = oprange[r];
			break;
		case AMOVWL:
			oprange[AMOVWZL] = oprange[r];
			break;
		case ADEC:
			oprange[AINC] = oprange[r];
			break;
		case ACOMB:
			oprange[ANEGB] = oprange[r];
			oprange[ASWAP] = oprange[r];
			oprange[ATST] = oprange[r];
			oprange[APUSHB] = oprange[r];
			oprange[APOPB] = oprange[r];
			oprange[ACLRB] = oprange[r];
			break;
		case ACOMW:
			oprange[ACLRW] = oprange[r];
			oprange[APUSHW] = oprange[r];
			break;
		case APOPW:
			break;
		case ACOML:
			oprange[ACLRL] = oprange[r];
			oprange[APUSHL] = oprange[r];
			break;
		case APOPL:
			break;
		case ANEGW:
		case ANEGL:
			break;
		case AEORB:
		case AEORW:
		case AEORL:
			break;
		case ACBRB:
			oprange[ASBRB] = oprange[r];
			break;
		case ABRBS:
			oprange[ABRBC] = oprange[r];
			break;
		case ABEQ:
			oprange[ABNE] = oprange[r];
			oprange[ABGE] = oprange[r];
			oprange[ABLT] = oprange[r];
			oprange[ABLO] = oprange[r];
			oprange[ABSH] = oprange[r];
			oprange[ABPL] = oprange[r];
			oprange[ABMI] = oprange[r];
			oprange[ABCC] = oprange[r];
			oprange[ABCS] = oprange[r];
			oprange[ABHC] = oprange[r];
			oprange[ABHS] = oprange[r];
			oprange[ABID] = oprange[r];
			oprange[ABIE] = oprange[r];
			oprange[ABTC] = oprange[r];
			oprange[ABTS] = oprange[r];
			oprange[ABVC] = oprange[r];
			oprange[ABVS] = oprange[r];
			break;
		case ABGT:
			oprange[ABLE] = oprange[r];
			oprange[ABHI] = oprange[r];
			oprange[ABSL] = oprange[r];
			break;
		case ABR:
			break;
		case ACMPB:
			oprange[ACMPBC] = oprange[r];
			break;
		case ACMPW:
		case ACMPL:
			break;
		case ACMPSE:
			break;
		case ASBRC:
			oprange[ASBRS] = oprange[r];
			break;
		case ASBIC:
			oprange[ASBIS] = oprange[r];
			break;
		case ABCLR:
			oprange[ABSET] = oprange[r];
			break;
		case ACBI:
			oprange[ASBI] = oprange[r];
			break;
		case ABLD:
			oprange[ABST] = oprange[r];
			break;
		case AIN:
			oprange[AOUT] = oprange[r];
			break;
		case AMOVPMB:
			oprange[AMOVPM] = oprange[r];
			break;
		case AMOVB:
		case AMOVW:
		case AMOVL:
		case AMOVPMW:
		case AMOVPML:
		case ACALL:
		case AWORD:
		case ANOOP:
		case ATEXT:
		case ADATA:
		case AVECTOR:
		case ASAVE:
		case ARESTORE:
			break;
		}
	}
}
