#include	"l.h"

#define	B(r)	(1<<(r))
#define	W(r)	(3<<(r))
#define	L(r)	(15<<(r))
#define	BWL(r, n)	((n)<<(r))

static	int	ccsign(Prog*);
static	int	isbr(int);

typedef struct Shifttab Shifttab;

struct Shifttab{
	char op;
	char sz;
	char *code;
};

/*
	& and <imm>
	- sbc <regoff>
	< rol
	> ror
	l bld <bitno>
	s bst <bitno>
	A asr <regoff>
	C clr <regoff>
	L lsl <regoff>
	M move <regoff> <regoff>
	N noop
	O op
	R lsr <regoff>
	X swap
*/

/* fill in more later if heavily used */
Shifttab shifttab[] =
{
	ALSLB,	0,	"N",
	ALSLB,	1,	"",
	ALSLB,	2,	"OO",
	ALSLB,	3,	"OOO",
	ALSLB,	4,	"X&f0",
	ALSLB,	5,	"XO&e0",
	ALSLB,	6,	"XOO&c0",
	ALSLB,	7,	">C0>",
	ALSLB,	-8,	"C0",
	ALSRB,	0,	"N",
	ALSRB,	1,	"",
	ALSRB,	2,	"OO",
	ALSRB,	3,	"OOO",
	ALSRB,	4,	"X&f",
	ALSRB,	5,	"XO&7",
	ALSRB,	6,	"XOO&3",
	ALSRB,	7,	"<C0<",
	ALSRB,	-8,	"C0",
	AASRB,	0,	"N",
	AASRB,	1,	"",
	AASRB,	2,	"OO",
	AASRB,	3,	"OOO",
	AASRB,	4,	"OOOO",
	AASRB,	5,	"OOOOO",
	AASRB,	6,	"s6L0-0l0",
	AASRB,	7,	"L0-0",
	AASRB,	-8,	"C0",
	ALSLW,	0,	"N",
	ALSLW,	1,	"",
	ALSLW,	2,	"OO",
	ALSLW,	8,	"M01C0",
	ALSLW,	10,	"M01C0L1L1",
	ALSLW,	-16,	"C0C1",
	ALSRW,	0,	"N",
	ALSRW,	1,	"",
	ALSRW,	2,	"OO",
	ALSRW,	8,	"M10C1",
	ALSRW,	10,	"M10C1R0R0",
	ALSRW,	-16,	"C0C1",
	AASRW,	0,	"N",
	AASRW,	1,	"",
	AASRW,	2,	"OO",
	AASRW,	7,	"L0M10<-1",
	AASRW,	8,	"M10L1-1",
	AASRW,	10,	"M10L1-1A0A0",
	AASRW,	-16,	"C0C1",
	ALSLL,	0,	"N",
	ALSLL,	1,	"",
	ALSLL,	2,	"OO",
	ALSLL,	8,	"M23M12M01C0",
	ALSLL,	16,	"M13M02C1C0",
	ALSLL,	24,	"M03C2C1C0",
	ALSLL,	-32,	"C0C1C2C3",
	ALSRL,	0,	"N",
	ALSRL,	1,	"",
	ALSRL,	2,	"OO",
	ALSRL,	3,	"OOO",
	ALSRL,	4,	"OOOO",
	ALSRL,	8,	"M10M21M32C3",
	ALSRL,	16,	"M20M31C2C3",
	ALSRL,	24,	"M30C1C2C3",
	ALSRL,	-32,	"C0C1C2C3",
	AASRL,	0,	"N",
	AASRL,	1,	"",
	AASRL,	2,	"OO",
	AASRL,	8,	"M10M21M32L3-3",
	AASRL,	16,	"M20M31L3-3-2",
	AASRL,	24,	"M30L3-3-2-1",
	AASRL,	-32,	"C0C1C2C3",
	AXXX,
};

static int
ispm(Adr *a)
{
	Sym *s;
	int t;

	s = a->sym;
	t = a->type;
	if(s != S && (s->type == SPMDATA || s->type == SPMBSS)){
		if(t != D_OREG && t != D_CONST)
			diag("bad program memory address");
		if(a->name != D_STATIC)
			diag("extern program memory data");
		return 1;
	}
	return 0;
}

static int
movpm(int a)
{
	switch(a){
	case AMOVB:	return AMOVPMB;
	case AMOVW:	return AMOVPMW;
	case AMOVL:	return AMOVPML;
	default:
		diag("bad op %A in movpm");
		break;
	}
	return a;
}

static int
movsz(int a)
{
	switch(a){
	case AMOVB:
	case AMOVPM:
	case AMOVPMB:
		return 1;
	case AMOVW:
	case AMOVBW:
	case AMOVBZW:
	case AMOVPMW:
		return 3;
	case AMOVL:
	case AMOVBL:
	case AMOVBZL:
	case AMOVPML:
	case AMOVWL:
	case AMOVWZL:
		return 15;
	default:
		diag("bad op %A in movsz");
		break;
	}
	return 0;
}

static int
iscon(Adr *a, long *v)
{
	*v = a->offset;
	return a->type == D_CONST && a->name == D_NONE;
}

static int
iszero(Adr *a)
{
	long	v;

	return a->type == D_REG && a->reg == REGZERO || iscon(a, &v) && v == 0;
}

Prog*
newprg(Prog *p)
{
	Prog *q;

	q = prg();
	q->line = p->line;
	q->link = p->link;
	p->link = q;
	return q;
}

static void
reg(Adr *a, int r)
{
	a->type = D_REG;
	a->reg = r;
}

void
con(Adr *a, int v)
{
	a->type = D_CONST;
	a->name = D_NONE;
	a->offset = v;
	a->reg = NREG;
	a->sym = S;
}

static void
cpprg(Prog *p, Prog *q)
{
	Prog *r;

	r = p->link;
	*p = *q;
	p->link = r;
}

Prog*
copyprg(Prog *p, Prog *q)
{
	Prog *r;

	r = newprg(p);
	cpprg(r, q);
	return r;
}

static void
and(Prog *p, int m, int r)
{
	Prog *q;

	switch(m){
	case 0:
		q = newprg(p);
		/* q->as = ACLRB; */
		q->as = AMOVB;
		reg(&q->from, REGZERO);
		reg(&q->to, r);
		break;
	case 255:
		break;
	default:
		q = newprg(p);
		q->as = AANDB;
		con(&q->from, m);
		reg(&q->to, r);
		break;
	}	
}

static void
or(Prog *p, int m, int r)
{
	Prog *q;

	switch(m){
	case 255:
		q = newprg(p);
		q->as = AMOVB;
		con(&q->from, 255);
		reg(&q->to, r);
		break;
	case 0:
		break;
	default:
		q = newprg(p);
		q->as = AORB;
		con(&q->from, m);
		reg(&q->to, r);
		break;
	}	
}

static int
logop(Prog *p, Prog *prv)
{
	int r;
	long v;

	if(!iscon(&p->from, &v))
		return 0;
	r = p->to.reg;
	switch(p->as){
	case AANDW:
		and(p, (v>>8)&0xff, r+1);
		and(p, (v>>0)&0xff, r);
		break;
	case AORW:
		or(p, (v>>8)&0xff, r+1);
		or(p, (v>>0)&0xff, r);
		break;
	case AANDL:
		and(p, (v>>24)&0xff, r+3);
		and(p, (v>>16)&0xff, r+2);
		and(p, (v>>8)&0xff, r+1);
		and(p, (v>>0)&0xff, r);
		break;
	case AORL:
		or(p, (v>>24)&0xff, r+3);
		or(p, (v>>16)&0xff, r+2);
		or(p, (v>>8)&0xff, r+1);
		or(p, (v>>0)&0xff, r);
		break;
	}
	prv->link = p->link;
	return 1;
}

static void
zero(Adr *a)
{
	a->type = D_REG;
	a->reg = REGZERO;
}

static void
incr(Adr *a, int i)
{
	switch(a->type){
	case D_REG:
		a->reg += i;
		break;
	case D_OREG:
		a->offset += i;
		break;
	case D_CONST:
		if(a->name == D_NONE){
			switch(i){
			case 3:
				a->offset = (a->offset>>24)&0xff;
				break;
			case 2:
				a->offset = (a->offset>>16)&0xffff;
				break;
			case 1:
				a->offset = (a->offset>>8)&0xff;
				break;
			}
		}
		else
			a->offset += i;
		break;
	}
}

static void
increm(Prog *p, int i)
{
	if(i == 3){
		incr(&p->from, 2);
		incr(&p->to, 2);
		i = 1;
	}
	incr(&p->from, i);
	incr(&p->to, i);
}
	
static void
cmpw(Prog *p)
{
	Prog *q, *r;

	p->as = ACMPB;
	q = p->link;
	if(p->from.type == D_REG){
		switch(q->as){
		case ABEQ:
		case ABNE:
		case ABGE:
		case ABLT:
		case ABSH:
		case ABLO:
			q = copyprg(p, p);
			q->as = ACMPBC;
			increm(q, 1);
			return;
		}
	}
	if(q->as == ABEQ || q->as == ABNE){
		r = newprg(q);
		cpprg(r, p);
		r = newprg(r);
		cpprg(r, q);
		if(q->as == ABEQ)
			q->pcond = r->link;
		q->as = ABNE;
	}
	else{
		r = newprg(q);
		cpprg(r, q);
		r->as = ABNE;
		r = newprg(r);
		cpprg(r, p);
		r = newprg(r);
		cpprg(r, q);
		r->as = relu(q->as);
		q->as = relne(q->as);
		q->link->pcond = r->link;
	}
	increm(p, 1);
}

static void
cmpl(Prog *p)
{
	int a;
	Prog *q, *r;

	p->as = ACMPB;
	q = p->link;
	a = q->as;
	if(p->from.type == D_REG){
		switch(a){
		case ABEQ:
		case ABNE:
		case ABGE:
		case ABLT:
		case ABSH:
		case ABLO:
			q = copyprg(p, p);
			q->as = ACMPBC;
			increm(q, 1);
			q = copyprg(q, q);
			increm(q, 1);
			q = copyprg(q, q);
			increm(q, 1);
			return;
		}
	}
	if(a == ABEQ || a == ABNE){
		q->as = ABNE;
		r = copyprg(q, p);
		r = copyprg(r, q);
		r = copyprg(r, p);
		r = copyprg(r, q);
		r = copyprg(r, p);
		r = copyprg(r, q);
		if(a == ABEQ){
			q->pcond = q->link->link->pcond = q->link->link->link->link->pcond = r->link;
			r->as = ABEQ;
		}
		increm(p, 3);
		increm(p->link->link, 2);
		increm(p->link->link->link->link, 1);
		return;
	}
	q->as = relne(a);
	r = copyprg(q, q);
	r->as = ABNE;
	r = copyprg(r, p);
	r = copyprg(r, q);
	r->as = relu(relne(a));
	r = copyprg(r, q);
	r->as = ABNE;
	r = copyprg(r, p);
	r = copyprg(r, q);
	r->as = relu(relne(a));
	r = copyprg(r, q);
	r->as = ABNE;
	r = copyprg(r, p);
	r = copyprg(r, q);
	r->as = relu(a);
	q->link->pcond = q->link->link->link->link->pcond = q->link->link->link->link->link->link->link->pcond = r->link;
	increm(p, 3);
	increm(p->link->link->link, 2);
	increm(p->link->link->link->link->link->link, 1);
}

static void
ext(Prog *p, int e)
{
	int i, n, r;
	Prog *q;

	n = p->from.type == D_REG ? p->from.reg : p->to.reg;
	p->as = e == 2 ? AMOVW : AMOVB;
	q = newprg(p);
	q->as = AMOVB;
	reg(&q->from, e == 2 ? n+1 : n);
	reg(&q->to, REGTMP);
	q = newprg(q);
	q->as = AROLB;
	reg(&q->to, REGTMP);
	r = p->to.reg+1;
	if(e == 2)
		r++;
	for(i = 0; i < e; i++, r++){
		q = newprg(q);
		q->as = ASUBBC;
		reg(&q->from, r);
		reg(&q->to, r);
	}
}

static int
imm(char **ss)
{
	int c, n;
	char *s;

	n = 0;
	s = *ss;
	for(;;){
		c = *s;
		switch(c){
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 16*n+c-'0';
			s++;
			break;
		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			n = 16*n+c-'a'+10;
			s++;
			break;
		default:
			goto endloop;
		}
	}
endloop:
	*ss = s;
	return n;
}

static int
regoffs(char **ss)
{
	int c;

	c = *(*ss)++;
	if(c < '0' || c > '3')
		diag("bad regoffs %d", c-'0');
	return c-'0';
}

static void
doshift(char *s, int as, int r, Prog *p, int f)
{
	int c, o, r0;
	Adr a;
	Prog *q;

	r0 = r;
	a = zprg.from;
	c = *s++;
	switch(c){
	case '\0':
		return;
	case '&':
		o = AANDB;
		con(&a, imm(&s));
		break;
	case '-':
		o = ASUBBC;
		r += regoffs(&s);
		reg(&a, r);
		break;
	case '<':
		o = AROLB;
		break;
	case '>':
		o = ARORB;
		break;
	case 'l':
		o = ABLD;
		con(&a, imm(&s));
		break;
	case 's':
		o = ABST;
		con(&a, imm(&s));
		break;
	case 'A':
		o = AASRB;
		r += regoffs(&s);
		break;
	case 'C':
		/* o = ACLRB; */
		o = AMOVB;
		reg(&a, REGZERO);
		r += regoffs(&s);
		break;
	case 'L':
		o = ALSLB;
		r += regoffs(&s);
		break;
	case 'M':
		o = AMOVB;
		reg(&a, r+regoffs(&s));
		r += regoffs(&s);
		break;
	case 'N':
		o = ANOOP;
		break;
	case 'O':
		o = as;
		break;
	case 'R':
		o = ALSRB;
		r += regoffs(&s);
		break;
	case 'X':
		o = ASWAP;
		break;
	default:
		o = AXXX;
		diag("bad doshift code");
		break;
	}
	if(f)
		q = p;
	else
		q = newprg(p);
	q->as = o;
	q->from = a;
	reg(&q->to, r);
	doshift(s, as, r0, q, 0);
}

static int
shift(Prog *p)
{
	int a, n;
	Shifttab *t;

	a = p->as;
	n = p->from.offset;
	for(t = shifttab; t->op != AXXX; t++){
		if(a == t->op){
			if(n == t->sz || (t->sz < 0 && n >= -t->sz)){
				doshift(t->code, a, p->to.reg, p, 1);
				return 1;
			}
		}
	}
	return 0;
}

static void
shiftloop(Prog *p)
{
	int i, n;
	Prog *r;

	n = p->from.offset;
	if(n <= 1 || n >= 32)
		diag("shift of %d in shiftloop: %P\n", n, p);
	p->from.offset = 1;
	p->from.type = D_NONE;
	if(n <= 4){
		r = p;
		for(i = 1; i < n; i++)
			r = copyprg(r, r);
		return;
	}
	r = newprg(p);
	cpprg(r, p);
	p->as = AMOVB;
	con(&p->from, n);
	reg(&p->to, REGTMP);
	r = newprg(r);
	r->as = ADEC;
	reg(&r->to, REGTMP);
	r = newprg(r);
	r->as = ABNE;
	r->to.type = D_BRANCH;
	r->pcond = p->link;
}

static void
mov2pm(Prog *p, long rm, long off)
{
	int r, n;
	Adr *a, *b;

	for( ; p != P && !(p->mark & VIS); p = p->link){
		p->mark |= VIS;
		if(rm == 0 && off == 0)
			return;
// print("mov2pm: %P %lux %lux\n", p, rm, off);
		if(isbr(p->as)){
			mov2pm(p->pcond, rm, off);
			if(p->as == ABR)
				return;
		}
		a = &p->from;
		b = &p->to;
		switch(p->as){
		case ATEXT:
		// case ACALL:
		case ARET:
		case ARETI:
		case ABREAK:
			return;
		case AADDW:
		case ASUBW:
			if(a->type == D_REG && (rm & B(a->reg)))
				rm |= B(b->reg);
			break;
		case AMOVW:
			if(a->type == D_REG && (rm & B(a->reg)) && b->type == D_REG){
				rm |= B(b->reg);
				break;
			}
			if(a->type == D_OREG && a->name == D_AUTO && off == a->offset){
				rm |= B(b->reg);
				break;
			}
			if(b->type == D_OREG && b->name == D_AUTO && (rm & B(a->reg))){
				off = b->offset;
				break;
			}
			/* FALLTHROUGH */
		case AMOVB:
		case AMOVL:
			if(a->type == D_OREG && a->name == D_NONE && (rm & B(a->reg)))
				p->as = movpm(p->as);
			/* FALLTHROUGH */
		case AMOVBL:
		case AMOVBW:
		case AMOVBZL:
		case AMOVBZW:
		case AMOVPM:
		case AMOVPMB:
		case AMOVPML:
		case AMOVPMW:
		case AMOVWL:
		case AMOVWZL:
			if(b->type == D_REG){
				r = b->reg;
				n = movsz(p->as);
				if(r&1)
					--r;
				if(rm & BWL(r, n))
					rm &= ~BWL(r, n);
			}
			break;
		default:
			break;
		}
	}
}

static void
clrvis(Prog *p)
{
	for( ; p != P && (p->mark & VIS); p = p->link){
		p->mark &= ~VIS;
		if(isbr(p->as)){
			clrvis(p->pcond);
			if(p->as == ABR)
				return;
		}
	}
}

void
noops(void)
{
	Prog *p, *q, *r, *q1;
	int o, curframe;
	long v;

	/*
	 * find leaf subroutines
	 * frame sizes
	 * strip NOPs
	 * expand RET
	 * expand BECOME pseudo
	 */

	if(debug['v'])
		Bprint(&bso, "%5.2f noops\n", cputime());
	Bflush(&bso);

	curframe = 0;
	curtext = 0;

	q = P;
	for(p = firstp; p != P; p = p->link) {
		/* find out how much arg space is used in this TEXT */
		if(p->to.type == D_OREG && p->to.reg == REGSP)
			if(p->to.offset > curframe)
				curframe = p->to.offset;

// print("noop: %P\n", p);

		if(ispm(&p->from)){
			switch(p->as){
			case AMOVB:
			case AMOVW:
			case AMOVL:
				if(p->from.type == D_OREG)
					p->as = movpm(p->as);
				else{
					if(p->to.type != D_REG)
						diag("not register in movpm");
					if(p->as != AMOVW)
						diag("not movw on address");
					mov2pm(p->link, B(p->to.reg), 0);
					clrvis(p->link);
				}
				break;
			default:
				diag("bad program memory instruction %P", p);
				break;
			}
		}
		if(ispm(&p->to))
			diag("writing to program memory");

		switch(p->as) {
		case ATEXT:
			if(curtext && curtext->from.sym)
				curtext->from.sym->frame = curframe;
			curframe = 0;
			p->mark |= LEAF;
			curtext = p;
			break;
		case ACALL:
			if(curtext != P)
				curtext->mark &= ~LEAF;
		case ABLE:
		case ABGT:
		case ABSL:
		case ABHI:
			if((q->as == ACMPB || q->as == ACMPW || q->as == ACMPL) && q->from.type == D_REG){
				o = q->from.reg;
				q->from.reg = q->to.reg;
				q->to.reg = o;
				p->as = relinv(relop(p->as));
			}
			/* FALLTHROUGH */
		case ABR:
		case ABEQ:
		case ABNE:
		case ABGE:
		case ABLT:
		case ABSH:
		case ABLO:
		case ABPL:
		case ABMI:
		case ABCC:
		case ABCS:
		case ABHC:
		case ABHS:
		case ABTC:
		case ABTS:
		case ABVC:
		case ABVS:
		case ABID:
		case ABIE:
		case AVECTOR:
			q1 = p->pcond;
			if(q1 != P) {
				while(q1->as == ANOOP) {
					q1 = q1->link;
					p->pcond = q1;
				}
			}
			break;
		case ARET:
		case ARETI:
			break;
		case ANOOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;
		case ACLRB:
			p->as = AMOVB;
			reg(&p->from, REGZERO);
			break;
		case ACLRW:
			if(!(p->to.reg & 1)){
				p->as = AMOVW;
				reg(&p->from, REGZERO);
			}
			break;
		case ACLRL:
			if(!(p->to.reg & 1)){
				p->as = AMOVW;
				reg(&p->from, REGZERO);
				r = copyprg(p, p);
				incr(&r->to, 2);
			}
			break;
		case AMOVB:
			if(iscon(&p->from, &v)){
				v &= 0xff;
				p->from.offset = v;
				if(v == 0 && p->to.type == D_REG)
					zero(&p->from);
			}
			if(p->from.type == D_REG && p->to.type == D_REG && p->from.reg == p->to.reg){
				q->link = q1 = p->link;
				q1->mark |= p->mark;
				continue;
			}
			break;
		case ACMPB:
			if(iscon(&p->from, &v))
				p->from.offset &= 0xff;
			/* FALL THROUGH */
		case AADDB:
		case AADDBC:
		case AADDW:
		case ASUBB:
		case ASUBBC:
		case ASUBW:
		case AANDB:
		case AORB:
			if(iscon(&p->from, &v) && v == 0)
				zero(&p->from);
			break;
		case ACBRB:
		case ACBRW:
		case ACBRL:
			p->as += AANDB-ACBRB;
			p->from.offset = ~p->from.offset;
			if(logop(p, q))
				continue;
			break;
		case ASBRB:
		case ASBRW:
		case ASBRL:
			p->as += AORB-ASBRB;
			if(logop(p, q))
				continue;
			break;
		case AANDW:
		case AANDL:
		case AORW:
		case AORL:
			if(logop(p, q))
				continue;
			break;
		case ALSLL:
		case ALSRL:
		case AASRL:
			if(p->from.type == D_CONST && p->from.offset == 10){
				r = copyprg(p, p);
				p->from.offset = 8;
				r->from.offset = 2;
			}
			/* FALLTHROUGH */
		case ALSLB:
		case ALSLW:
		case ALSRB:
		case ALSRW:
		case AASRB:
		case AASRW:
			o = p->from.type;
			if(p->from.type == D_CONST){
				if(p->from.offset == 0){
					q->link = p->link;
					continue;
				}
				p->from.type = D_NONE;
			}
			else if(p->from.type != D_NONE)
				diag("illegal lhs in %P", p);
			else
				p->from.offset = 1;
			if(!shift(p)){
				p->from.type = o;
				shiftloop(p);	/* can happen sometimes */
				// diag("cannot do shift %P", p);
			}
			break;
		case AROLB:
		case AROLW:
		case AROLL:
		case ARORB:
		case ARORW:
		case ARORL:
			if(p->from.type == D_CONST){
				if(p->from.offset == 0){
					q->link = p->link;
					continue;
				}
				if(p->from.offset != 1)
					diag("rotate shift not 1");
				p->from.type = D_NONE;
			}
			else if(p->from.type != D_NONE)
				diag("illegal lhs in %P", p);
			else
				p->from.offset = 1;
			break;
		case ACMPW:
			if(iszero(&p->from) && ccsign(p->link)){
				p->as = ACMPB;
				incr(&p->to, 1);
			}
			else{
				q1 = p->link;
				if(q1->to.type != D_BRANCH)
					diag("cmpw not followed by a branch");
				cmpw(p);
			}
			p = q;
			continue;
		case ACMPL:
			if(iszero(&p->from) && ccsign(p->link)){
				p->as = ACMPB;
				incr(&p->to, 3);
			}
			else{
				q1 = p->link;
				if(q1->to.type != D_BRANCH)
					diag("cmpl not followed by a branch");
				cmpl(p);
			}
			p = q;
			continue;
		case AADDL:
			if(iscon(&p->from, &v)){
				p->from.offset = -p->from.offset;
				p->as = ASUBL;
			}
			break;
		case AMOVW:
			if(p->from.type == D_OREG && p->from.reg == p->to.reg)
				break;
			if(p->to.type == D_OREG && p->from.reg == p->to.reg)
				break;
			if(iscon(&p->from, &v) && v == 0 && p->to.type == D_REG)
				zero(&p->from);
			if(iscon(&p->from, &v) || p->from.type == D_CONST)
				break;
			if(p->from.type == D_REG && p->to.type == D_REG){
				if(p->from.reg == p->to.reg){
					q->link = q1 = p->link;
					q1->mark |= p->mark;
					continue;
				}
				if(!(p->from.reg&1) && !(p->to.reg&1))
					break;	/* both even */
			}
			if(p->from.type == D_OREG && p->from.name == D_NONE && p->from.reg != REGZ && p->to.reg != REGZ && p->from.reg != REGY){
				r = copyprg(p, p);
				p->from.type = D_REG;
				p->to.reg = REGZ;
				r->from.reg = REGZ;
				break;
			}
			if(p->to.type == D_OREG && p->to.name == D_NONE && p->to.reg != REGZ && p->from.reg != REGZ && p->to.reg != REGY){
				r = copyprg(p, p);
				p->from.reg = p->to.reg;
				p->to.reg = REGZ;
				p->to.type = D_REG;
				r->to.reg = REGZ;
				break;
			}
			r = newprg(p);
			p->as = AMOVB;
			cpprg(r, p);
			increm(r, 1);
			break;
		case AMOVL:
			if(p->from.type == D_OREG && p->from.reg == p->to.reg)
				break;
			if(p->to.type == D_OREG && p->from.reg == p->to.reg)
				break;
			if(p->from.type == D_OREG && p->from.name == D_NONE && p->from.reg != REGZ && p->to.reg != REGZ && p->from.reg != REGY){
				r = copyprg(p, p);
				p->as = AMOVW;
				p->from.type = D_REG;
				p->to.reg = REGZ;
				r->from.reg = REGZ;
				break;
			}
			if(p->to.type == D_OREG && p->to.name == D_NONE && p->to.reg != REGZ && p->from.reg != REGZ && p->to.reg != REGY){
				r = copyprg(p, p);
				p->as = AMOVW;
				p->from.reg = p->to.reg;
				p->to.reg = REGZ;
				p->to.type = D_REG;
				r->to.reg = REGZ;
				break;
			}
			r = newprg(p);
			p->as = AMOVW;
			cpprg(r, p);
			if(iscon(&p->from, &v))
				p->from.offset = v&0xffff;
			increm(r, 2);
			p = q;
			continue;		/* expand AMOVW */
		case	AMOVBW:
			ext(p, 1);
			p = q;
			continue;
		case	AMOVWL:
			ext(p, 2);
			p = q;
			continue;
		case	AMOVBL:
			ext(p, 3);
			p = q;
			continue;
		case AMOVBZW:
			r = newprg(p);
			p->as = AMOVB;
			cpprg(r, p);
			zero(&r->from);
			incr(&r->to, 1);
			p = q;
			continue;
		case AMOVBZL:
			r = newprg(p);
			p->as = AMOVBZW;
			cpprg(r, p);
			r->as = AMOVW;
			zero(&r->from);
			incr(&r->to, 2);
			p = q;
			continue;	/* expand further */
		case AMOVWZL:
			r = newprg(p);
			p->as = AMOVW;
			cpprg(r, p);
			zero(&r->from);
			incr(&r->to, 2);
			p = q;
			continue;	/* expand further */
		default:
			break;
		}
		q = p;
	}
	if(curtext && curtext->from.sym)
		curtext->from.sym->frame = curframe;

	curtext = 0;
	for(p = firstp; p != P; p = p->link) {
		switch(p->as) {
		case ATEXT:
			curtext = p;
			break;
		case ACALL:
			if(curtext != P && curtext->from.sym != S && curtext->to.offset >= 0) {
				o = 0 - curtext->from.sym->frame;
				if(o <= 0)
					break;
				/* calling a variable */
				if(p->to.sym == S) {
					curtext->to.offset += o;
					if(debug['b']) {
						curp = p;
						print("%D calling %D increase %d\n",
							&curtext->from, &p->to, o);
					}
				}
			}
			break;
		}
	}

	curtext = P;
	for(p = firstp; p != P; p = p->link) {
		o = p->as;
		switch(o) {
		case ATEXT:
			curtext = p;
			autosize = p->to.offset + AUTINC;
			if(autosize > 0){
				q = newprg(p);
				q->as = ASUBW;
				con(&q->from, autosize);
				reg(&q->to, REGSP);
			}
			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
			}
			break;
		case ARET:
		case ARETI:
			if(autosize > 0){
				p->as = AADDW;
				con(&p->from, autosize);
				reg(&p->to, REGSP);
				p = newprg(p);
				p->as = o;
			}
			break;
		}
	}
}

void
addnop(Prog *p)
{
	Prog *q;

	q = newprg(p);
	q->as = ANOOP;
	reg(&q->from, REGZERO);
	reg(&q->to, REGZERO);
}

static int
ismov(int a)
{
	switch(a){
	case AMOVB:
	case AMOVBL:
	case AMOVBW:
	case AMOVBZL:
	case AMOVBZW:
	case AMOVL:
	case AMOVPM:
	case	AMOVPMB:
	case AMOVPML:
	case AMOVPMW:
	case AMOVW:
	case AMOVWL:
	case AMOVWZL:
		return 1;
	}
	return 0;
}

static int
isbr(int a)
{
	switch(a){
	case ABRBS:
	case ABRBC:
	case ABR:
	case ABEQ:
	case ABNE:
	case ABCS:
	case ABCC:
	case ABSH:
	case ABSL:
	case ABLO:
	case ABMI:
	case ABPL:
	case ABGE:
	case ABLT:
	case ABHC:
	case ABHS:
	case ABTC:
	case ABTS:
	case ABVC:
	case ABVS:
	case ABID:
	case ABIE:
	case ABGT:
	case ABHI:
	case ABLE:
		return 1;
	}
	return 0;
}

static int
setcc(Prog *p)
{
	int a;

	if(p == P)
		return 1;
	a = p->as;
	if(ismov(a) || isbr(a))
		return 0;
	switch(a){
	case ABCLR:
	case ABLD:
	case ABSET:
	case ABST:
	case ACBI:
	case ACMPSE:
	case AFMULSB:
	case AFMULSUB:
	case AFMULUB:
	case AIN:
	case AMULB:
	case AMULUB:
	case AMULSUB:
	case AOUT:
	case APOPB:
	case APOPL:
	case APOPW:
	case APUSHB:
	case APUSHL:
	case APUSHW:
	case ASBI:
	case ASBIC:
	case ASBIS:
	case ASBRC:
	case ASBRS:
	case ASWAP:
	case ABREAK:
	case ANOP:
	case ASLEEP:
	case AWDR:
	case ANOOP:
	case AWORD:
		return 0;
	}
	return 1;
}

/*
 * is the current cc only ever used for a signed comparison with 0
 * a very conservative test is used
 */
static int
ccsign(Prog *p)
{
	if(p == P)
		return 1;
	if(p->as != ABGE && p->as != ABLT)
		return 0;
	return setcc(p->link) && setcc(p->pcond);
}
