#include "gc.h"

#define	AMVBZW	AMOVBZW
#define	AMVBZL	AMOVBZL
#define	AMVWZL	AMOVWZL

static uchar movtab[TIND+1][TIND+1] =
{
			/* TXXX	TCHAR	TUCHAR	TSHORT	TUSHORT	TINT		TUINT	TLONG	TULONG	TVLONG	TUVLONG	TFLOAT	TDOUBLE	TIND */
/* TXXX */	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,
/* TCHAR */	AXXX,	AMOVB,	AMOVB,	AMOVBW,	AMOVBW,AMOVBW,	AMOVBW,AMOVBL,	AMOVBL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVBW,
/* TUCHAR */	AXXX,	AMOVB,	AMOVB,	AMVBZW,	AMVBZW,	AMVBZW,	AMVBZW,	AMVBZL,	AMVBZL,	AXXX,	AXXX,	AXXX,	AXXX,	AMVBZW,
/* TSHORT */	AXXX,	AMOVB,	AMOVB,	AMOVW,	AMOVW,	AMOVW,	AMOVW,	AMOVWL,	AMOVWL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVW,
/* TUSHORT */	AXXX,	AMOVB,	AMOVB,	AMOVW,	AMOVW,	AMOVW,	AMOVW,	AMVWZL,	AMVWZL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVW,
/* TINT */	AXXX,	AMOVB,	AMOVB,	AMOVW,	AMOVW,	AMOVW,	AMOVW,	AMOVWL,	AMOVWL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVW,
/* TUINT */	AXXX,	AMOVB,	AMOVB,	AMOVW,	AMOVW,	AMOVW,	AMOVW,	AMVWZL,	AMVWZL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVW,
/* TLONG */	AXXX,	AMOVB,	AMOVB,	AMOVW,	AMOVW,	AMOVW,	AMOVW,	AMOVL,	AMOVL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVW,
/* TULONG */	AXXX,	AMOVB,	AMOVB,	AMOVW,	AMOVW,	AMOVW,	AMOVW,	AMOVL,	AMOVL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVW,
/* TVLONG */	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,
/* TUVLONG */	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,
/* TFLOAT */	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,
/* TDOUBLE */	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,	AXXX,
/* TIND */	AXXX,	AMOVB,	AMOVB,	AMOVW,	AMOVW,	AMOVW,	AMOVW,	AMOVWL,	AMOVWL,	AXXX,	AXXX,	AXXX,	AXXX,	AMOVW,
};

static int regarget = TXXX;

int
regget0(int et, long b, int opt, int hi, int min, int max)
{
	int i;
	long w;
	ulong m;

	w = ewidth[et];
	// print("regget: %d %ld %lux %d %d\n", et, w, b, opt, hi);
	if(et == TIND){
		m = 3L << REGX;
		for(i = REGX; i >= REGW; i -= 2){
			if((b & m) == m)
			if(reg[i] == 0 && reg[i+1] == 0)
				return i;
			m >>= 2;
		}
		if(opt)
			return 0;
	}
	switch(w){
	default:
		diag(Z, "bad size %ld in regget", w);
		return 0;
	case 4:
		if(hi){
			m = 15L << REGHI;
			for(i = REGHI; i < REGLAST-3; i += 2){
				if((b & m) == m)
				if(reg[i] == 0 && reg[i+1] == 0 && reg[i+2] == 0 && reg[i+3]  == 0)
					return i;
				m <<= 2;
			}
		}
		m = 15L << min;
		for(i = min; i < max-3; i += 2){
			if(i == 14)
				continue;	/* not mixture of high and low registers */
			if((b & m) == m)
			if(reg[i] == 0 && reg[i+1] == 0 && reg[i+2] == 0 && reg[i+3] == 0)
				return i;
			m <<= 2;
		}
		break;	/* insist on an even reg */
	case 2:
		hi = 1;
		if(hi){
			m = 3L << REGHI;
			for(i = REGHI; i < REGLAST-1; i += 2){
				if((b & m) == m)
				if(reg[i] == 0 && reg[i+1] == 0)
					return i;
				m <<= 2;
			}
		}
		m = 3L << min;
		for(i = min; i < max-1; i += 2){
			if((b & m) == m)
			if(reg[i] == 0 && reg[i+1] == 0)
				return i;
			m <<= 2;
		}
		break;	/* insist on an even reg */
	case 1:
		if(hi){
			m = 1L << REGHI;
			for(i = REGHI; i < REGLAST; i++){
				if((b & m) == m)
				if(reg[i] == 0)
					return i;
				m <<= 1;
			}
		}
		m = 1L << min;
		for(i = min; i < max; i++){
			if((b & m) == m)
			if(reg[i] == 0)
				return i;
			m <<= 1;
		}
		break;
	}
	return 0;
}

int
regget(int et, long b, int opt, int hi)
{
	int i;

	i = regget0(et, b, opt, hi, REGFIRST, REGLAST);
	if(i == 0 && opt == 0)
		i = regget0(et, b, 0, 0, REGW, REGY);
	return i;
}

void
reginc(int i, int et, Node *n)
{
	long w;

	w = ewidth[et];
	switch(w){
	case 4:
		reg[i+2]++;
		reg[i+3]++;
		/* FALLTHROUGH */
	case 2:
		reg[i+1]++;
		/* FALLTHROUGH */
	case 1:
		reg[i]++;
		break;
	default:
		diag(n, "bad reginc size: %ld\n", w);
		break;
	}
}

void
regdec(int i, int et, Node *n)
{
	long w;

	w = ewidth[et];
	switch(w){
	case 4:
		if(reg[i+2] <= 0)
			goto err;
		reg[i+2]--;
		if(reg[i+3] <= 0)
			goto err;
		reg[i+3]--;
		/* FALLTHROUGH */
	case 2:
		if(reg[i+1] <= 0)
			goto err;
		reg[i+1]--;
		/* FALLTHROUGH */
	case 1:
		if(reg[i] <= 0)
			goto err;
		reg[i]--;
		break;
	default:
		diag(n, "bad regdec size: %ld\n", w);
		break;
	}
	return;
err:
	diag(n, "bad regdec cnt %d %d %d %d %d %d\n", reg[i], reg[i+1], reg[i+2], reg[i+3], i, et);
}

void
regargfree(void)
{
	regdec(argreg(regarget), regarget, Z);
	regarget = TXXX;
}

static int
move(int ft, int tt)
{
	int a;

	a = AXXX;
	if(ft <= TIND && tt <= TIND)
		a = movtab[ft][tt];
	return a == AXXX ? AGOK : a;
}

void
ginit(void)
{
	Type *t;

	thechar = 'z';
	thestring = "avr";
	exregoffset = REGEXT;
	exfregoffset = FREGEXT;
	listinit();
	nstring = 0;
	mnstring = 0;
	nrathole = 0;
	pc = 0;
	breakpc = -1;
	continpc = -1;
	cases = C;
	firstp = P;
	lastp = P;
	tfield = types[TLONG];

	zprog.link = P;
	zprog.as = AGOK;
	zprog.size = 0;
	zprog.from.type = D_NONE;
	zprog.from.name = D_NONE;
	zprog.from.reg = NREG;
	zprog.to = zprog.from;

	regnode.op = OREGISTER;
	regnode.class = CEXREG;
	regnode.reg = REGTMP;
	regnode.complex = 0;
	regnode.addable = 11;
	regnode.type = types[TLONG];

	constnode.op = OCONST;
	constnode.class = CXXX;
	constnode.complex = 0;
	constnode.addable = 20;
	constnode.type = types[TLONG];

	fconstnode.op = OCONST;
	fconstnode.class = CXXX;
	fconstnode.complex = 0;
	fconstnode.addable = 20;
	fconstnode.type = types[TDOUBLE];

	nodsafe = new(ONAME, Z, Z);
	nodsafe->sym = slookup(".safe");
	nodsafe->type = types[TINT];
	nodsafe->etype = types[TINT]->etype;
	nodsafe->class = CAUTO;
	complex(nodsafe);

	t = typ(TARRAY, types[TCHAR]);
	symrathole = slookup(".rathole");
	symrathole->class = CGLOBL;
	symrathole->type = t;

	nodrat = new(ONAME, Z, Z);
	nodrat->sym = symrathole;
	nodrat->type = types[TIND];
	nodrat->etype = TVOID;
	nodrat->class = CGLOBL;
	complex(nodrat);
	nodrat->type = t;

	nodret = new(ONAME, Z, Z);
	nodret->sym = slookup(".ret");
	nodret->type = types[TIND];
	nodret->etype = TIND;
	nodret->class = CPARAM;
	nodret = new(OIND, nodret, Z);
	complex(nodret);

	com64init();
	commulinit();

	memset(reg, 0, sizeof(reg));
	// reg[REGTMP] = 1;
}

void
gclean(void)
{
	int i;
	Sym *s;

	for(i=0; i<NREG; i++)
		if(reg[i])
			diag(Z, "reg %d left allocated", i);
	for(i=NREG; i<NREG+NFREG; i++)
		if(reg[i])
			diag(Z, "freg %d left allocated", i-NREG);
	while(mnstring)
		outstring("", 1L);
	symstring->type->width = nstring;
	symrathole->type->width = nrathole;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type == T)
			continue;
		if(s->type->width == 0)
			continue;
		if(s->class != CGLOBL && s->class != CSTATIC)
			continue;
		if(s->type == types[TENUM])
			continue;
		if(s->class == CSTATIC && lookupstatic(s)->class == CSTATIC){
			gpseudo(APM, s, nodconst(s->type->width, Z));
			print("%s in program memory\n", s->name);
		}
		else
			gpseudo(AGLOBL, s, nodconst(s->type->width, Z));
	}
	nextpc();
	p->as = AEND;
	outcode();
}

void
nextpc(void)
{

	p = alloc(sizeof(*p));
	*p = zprog;
	p->lineno = nearln;
	pc++;
	if(firstp == P) {
		firstp = p;
		lastp = p;
		return;
	}
	lastp->link = p;
	lastp = p;
}

void
gargs(Node *n, Node *tn1, Node *tn2)
{
	long regs;
	Node fnxargs[20], *fnxp;

	regs = cursafe;

	fnxp = fnxargs;
	garg1(n, tn1, tn2, 0, &fnxp);	/* compile fns to temps */

	curarg = 0;
	fnxp = fnxargs;
	garg1(n, tn1, tn2, 1, &fnxp);	/* compile normal args and temps */

	cursafe = regs;
}

void
garg1(Node *n, Node *tn1, Node *tn2, int f, Node **fnxp)
{
	Node nod;

	if(n == Z)
		return;
	if(n->op == OLIST) {
		garg1(n->left, tn1, tn2, f, fnxp);
		garg1(n->right, tn1, tn2, f, fnxp);
		return;
	}
	if(f == 0) {
		if(n->complex >= FNX) {
			regsalloc(*fnxp, n);
			nod = znode;
			nod.op = OAS;
			nod.left = *fnxp;
			nod.right = n;
			nod.type = n->type;
			cgen(&nod, Z);
			(*fnxp)++;
		}
		return;
	}
	if(typesuv[n->type->etype]) {
		regaalloc(tn2, n);
		if(n->complex >= FNX) {
			sugen(*fnxp, tn2, n->type->width);
			(*fnxp)++;
		} else
			sugen(n, tn2, n->type->width);
		return;
	}
	if(REGARG >= 0 && curarg == 0 && typechlp[n->type->etype]) {
		regaalloc1(tn1, n);
		if(n->complex >= FNX) {
			cgen(*fnxp, tn1);
			(*fnxp)++;
		} else
			cgen(n, tn1);
		return;
	}
	regalloc(tn1, n, Z, 0);
	if(n->complex >= FNX) {
		cgen(*fnxp, tn1);
		(*fnxp)++;
	} else
		cgen(n, tn1);
	regaalloc(tn2, n);
	gopcode(OAS, tn1, tn2);
	regfree(tn1);
}

Node*
nodconst(long v, Node *n)
{
	Type *t;

	if(n)
		t = n->type;
	else
		t = types[TLONG];
	constnode.vconst = v;
	constnode.type = t;
	return &constnode;
}

Node*
nod32const(vlong v, Node *n)
{
	Type *t;

	if(n)
		t = n->type;
	else
		t = types[TLONG];
	constnode.vconst = v & MASK(32);
	constnode.type = t;
	return &constnode;
}

Node*
nodfconst(double d)
{
	fconstnode.fconst = d;
	return &fconstnode;
}

void
nodreg(Node *n, Node *nn, int reg)
{
	*n = regnode;
	n->reg = reg;
	n->type = nn->type;
	n->lineno = nn->lineno;
}

void
regret(Node *n, Node *nn)
{
	int r;

	r = retreg(nn->type->etype);
	if(typefd[nn->type->etype])
		r = FREGRET+NREG;
	nodreg(n, nn, r);
	// reg[r]++;
	reginc(r, nn->type->etype, nn);
}

int
retreg(int et)
{
	if(ewidth[et] == SZ_LONG)
		return REGRETL;
	return REGRET;
}

int
argreg(int et)
{
	if(ewidth[et] == SZ_LONG)
		return REGARGL;
	return REGARG;
}

/*
int
tmpreg(void)
{
	int i;

	for(i=REGFREE; i<REGTMP; i++)
		if(reg[i] == 0)
			return i;
	diag(Z, "out of fixed tmp registers");
	return 0;
}
*/

void
regalloc(Node *n, Node *tn, Node *o, int hi)
{
	int i, j, et;
	long w;
	static lasti;

	et = tn->type->etype;
	w = ewidth[et];
	switch(et) {
	case TCHAR:
	case TUCHAR:
	case TSHORT:
	case TUSHORT:
	case TINT:
	case TUINT:
	case TLONG:
	case TULONG:
	case TIND:
		if(!hi)
			hi = sconst(tn);
/*
		if(o != Z && o->op == OREGISTER && w == ewidth[o->type->etype] && hi == hireg(o)) {
			i = o->reg;
			if(i >= 0 && i < NREG)
				goto out;
		}
*/
		if(o != Z && o->op == OREGISTER && w == ewidth[o->type->etype]) {
			i = o->reg;
			if(i >= 0 && i < NREG)
				goto out;
		}
		i = regget(et, ~0, 0, hi);
		if(i > 0)
			goto out;
		diag(tn, "out of fixed registers");
		goto err;

	case TFLOAT:
	case TDOUBLE:
	case TVLONG:
		if(o != Z && o->op == OREGISTER) {
			i = o->reg;
			if(i >= NREG && i < NREG+NFREG)
				goto out;
		}
		j = 0*2 + NREG;
		for(i=NREG; i<NREG+NFREG; i++) {
			if(j >= NREG+NFREG)
				j = NREG;
			if(reg[j] == 0) {
				i = j;
				goto out;
			}
			j++;
		}
		diag(tn, "out of float registers");
		goto err;
	}
	diag(tn, "unknown type in regalloc: %T", tn->type);
err:
	reginc(0, et, tn);
	nodreg(n, tn, 0);
	return;
out:
	reginc(i, et, tn);
	// reg[i]++;
	nodreg(n, tn, i);
}

void
regialloc(Node *n, Node *tn, Node *o)
{
	Node nod;

	nod = *tn;
	nod.type = types[TIND];
	regalloc(n, &nod, o, 0);
}

void
regfree(Node *n)
{
	int i, et;

	i = 0;
	if(n->op != OREGISTER && n->op != OINDREG)
		goto err;
	i = n->reg;
	if(i < 0 || i >= sizeof(reg))
		goto err;
	if(n->op == OINDREG)
		et = TIND;
	else
		et = n->type->etype;
	regdec(i, et, n);
	return;
err:
	diag(n, "error in regfree: %d", i);
}

void
regsalloc(Node *n, Node *nn)
{
	cursafe = align(cursafe, nn->type, Aaut3);
	maxargsafe = maxround(maxargsafe, cursafe+curarg);
	*n = *nodsafe;
	n->xoffset = -(stkoff + cursafe);
	n->type = nn->type;
	n->etype = nn->type->etype;
	n->lineno = nn->lineno;
}

void
regaalloc1(Node *n, Node *nn)
{
	int r;

	r = argreg(nn->type->etype);
	nodreg(n, nn, r);
	// reg[r]++;
	reginc(r, nn->type->etype, nn);
	curarg = align(curarg, nn->type, Aarg1);
	curarg = align(curarg, nn->type, Aarg2);
	maxargsafe = maxround(maxargsafe, cursafe+curarg);
	regarget = nn->type->etype;
}

void
regaalloc(Node *n, Node *nn)
{
	curarg = align(curarg, nn->type, Aarg1);
	*n = *nn;
	n->op = OINDREG;
	n->reg = REGSP;
	n->xoffset = curarg;
	n->complex = 0;
	n->addable = 20;
	curarg = align(curarg, nn->type, Aarg2);
	maxargsafe = maxround(maxargsafe, cursafe+curarg);
}

void
regind(Node *n, Node *nn)
{

	if(n->op != OREGISTER) {
		diag(n, "regind not OREGISTER");
		return;
	}
	n->op = OINDREG;
	n->type = nn->type;
}

static void
zadr(Adr *a)
{
	if(REGZERO >= 0 && a->type == D_CONST && a->name == D_NONE && a->offset == 0 && ewidth[a->etype] <= 2){
		a->type = D_REG;
		a->reg = REGZERO;
	}
}

static void
nzaddr(Node *n, Adr *a)
{
	naddr(n, a);
	zadr(a);
}

void
naddr(Node *n, Adr *a)
{
	long v;

	a->type = D_NONE;
	if(n == Z)
		return;
	a->etype = n->type->etype;
	switch(n->op) {
	default:
	bad:
		diag(n, "bad in naddr: %O", n->op);
		break;

	case OREGISTER:
		a->type = D_REG;
		a->sym = S;
		a->reg = n->reg;
		if(a->reg >= NREG) {
			a->type = D_FREG;
			a->reg -= NREG;
		}
		break;

	case OIND:
		naddr(n->left, a);
		if(a->type == D_REG) {
			a->type = D_OREG;
			break;
		}
		if(a->type == D_CONST) {
			a->type = D_OREG;
			break;
		}
		goto bad;

	case OINDREG:
		a->type = D_OREG;
		a->sym = S;
		a->offset = n->xoffset;
		a->reg = n->reg;
		break;

	case ONAME:
		a->etype = n->etype;
		a->type = D_OREG;
		a->name = D_STATIC;
		a->sym = n->sym;
		a->offset = n->xoffset;
		if(n->class == CSTATIC)
			break;
		if(n->class == CEXTERN || n->class == CGLOBL) {
			a->name = D_EXTERN;
			break;
		}
		if(n->class == CAUTO) {
			a->name = D_AUTO;
			break;
		}
		if(n->class == CPARAM) {
			a->name = D_PARAM;
			break;
		}
		goto bad;

	case OCONST:
		a->sym = S;
		a->reg = NREG;
		if(typefd[n->type->etype]) {
			a->type = D_FCONST;
			a->dval = n->fconst;
		} else {
			a->type = D_CONST;
			a->offset = n->vconst;
		}
		break;

	case OADDR:
		naddr(n->left, a);
		if(a->type == D_OREG) {
			a->type = D_CONST;
			break;
		}
		goto bad;

	case OADD:
		if(n->left->op == OCONST) {
			naddr(n->left, a);
			v = a->offset;
			naddr(n->right, a);
		} else {
			naddr(n->right, a);
			v = a->offset;
			naddr(n->left, a);
		}
		a->offset += v;
		break;

	}
}

void
gmove(Node *f, Node *t)
{
	int ft, tt, a;
	Node nod;

// prtree(f, "gmove src");
// prtree(t, "gmove dst");

	ft = f->type->etype;
	tt = t->type->etype;

	if(ft == TDOUBLE && f->op == OCONST) {
	}
	if(ft == TFLOAT && f->op == OCONST) {
	}

	/*
	 * a load --
	 * put it into a register then
	 * worry what to do with it.
	 */
	if(f->op == ONAME || f->op == OINDREG || f->op == OIND) {
		a = move(ft, tt);
		if(typechlp[ft] && typeilp[tt])
			regalloc(&nod, t, t, 0);
		else
			regalloc(&nod, f, t, 0);
		gins(a, f, &nod);
		gmove(&nod, t);
		regfree(&nod);
		return;
	}

	/*
	 * a store --
	 * put it into a register then
	 * store it.
	 */
	if(t->op == ONAME || t->op == OINDREG || t->op == OIND) {
		a = move(ft, tt);
		if(ft == tt)
			regalloc(&nod, t, f, 0);
		else
			regalloc(&nod, t, Z, 0);
		gmove(f, &nod);
		gins(a, &nod, t);
		regfree(&nod);
		return;
	}

	/*
	 * type x type cross table
	 */
	a = move(ft, tt);
	if(a == AGOK)
		diag(Z, "bad opcode in gmove %T -> %T", f->type, t->type);
	if(ewidth[ft] == ewidth[tt])
	if(samaddr(f, t))
		return;
	if(f->op == OCONST && !typefd[f->type->etype] && f->vconst == 0 && t->op == OREGISTER && !typefd[t->type->etype]){
		switch(a){
		case AMOVL:
		case AMOVBL:
		case AMOVBZL:
		case AMOVWL:
		case AMOVWZL:
			a = ACLRL;
			break;
		case AMOVW:
		case AMOVBW:
		case AMOVBZW:
			a = ACLRW;
			break;
		case AMOVB:
			a = ACLRB;
			break;
		}
		gins(a, Z, t);
		return;
	}
	gins(a, f, t);
}

void
gins(int a, Node *f, Node *t)
{

	nextpc();
	p->as = a;
	if(f != Z)
		nzaddr(f, &p->from);
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
}

void
gopcode(int o, Node *f, Node *t)
{
	int a, et, mul, rr, swap;

	mul = 0;
	et = TLONG;
	if(f != Z && f->type != T)
		et = f->type->etype;
	if(f == Z && t->type != T)
		et = t->type->etype;
	a = AGOK;
	switch(o) {
	case ONEG:
		if(f != Z)
			diag(Z, "bad operands for ONEG");
		f = Z;
		a = ANEGW;
		if(et == TUCHAR || et == TCHAR)
			a = ANEGB;
		else if(et == TULONG || et == TLONG)
			a = ANEGL;
		break;

	case OCOM:
		if(f != Z)
			diag(Z, "bad operands for OCOM");
		f = Z;
		a = ACOMW;
		if(et == TUCHAR || et == TCHAR)
			a = ACOMB;
		else if(et == TULONG || et == TLONG)
			a = ACOML;
		break;

	case OAS:
		gmove(f, t);
		return;

	case OASADD:
	case OADD:
		a = AADDW;
		if(et == TUCHAR || et == TCHAR)
			a = AADDB;
		else if(et == TULONG || et == TLONG)
			a = AADDL;
/*
		if(et == TFLOAT)
			a = AADDF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = AADDD;
*/
		if(o == OADD && a == AADDB && f->op == OCONST && f->vconst == 1){
			a = AINC;
			f = Z;
		}
		break;

	case OASSUB:
	case OSUB:
		a = ASUBW;
		if(et == TUCHAR || et == TCHAR)
			a = ASUBB;
		else if(et == TULONG || et == TLONG)
			a = ASUBL;
/*
		if(et == TFLOAT)
			a = ASUBF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = ASUBD;
*/
		if(o == OSUB && a == ASUBB && f->op == OCONST && f->vconst == 1){
			a = ADEC;
			f = Z;
		}
		break;

	case OASOR:
	case OOR:
		a = AORW;
		if(et == TUCHAR || et == TCHAR)
			a = AORB;
		else if(et == TULONG || et == TLONG)
			a = AORL;
		break;

	case OASAND:
	case OAND:
		a = AANDW;
		if(et == TUCHAR || et == TCHAR)
			a = AANDB;
		else if(et == TULONG || et == TLONG)
			a = AANDL;
		break;

	case OASXOR:
	case OXOR:
		a = AEORW;
		if(et == TUCHAR || et == TCHAR)
			a = AEORB;
		else if(et == TULONG || et == TLONG)
			a = AEORL;
		break;

	case OASLSHR:
	case OLSHR:
		a = ALSRW;
		if(et == TUCHAR || et == TCHAR)
			a = ALSRB;
		else if(et == TULONG || et == TLONG)
			a = ALSRL;
		break;

	case OASASHR:
	case OASHR:
		a = AASRW;
		if(et == TUCHAR || et == TCHAR)
			a = AASRB;
		else if(et == TULONG || et == TLONG)
			a = AASRL;
		break;

	case OASASHL:
	case OASHL:
		a = ALSLW;
		if(et == TUCHAR || et == TCHAR)
			a = ALSLB;
		else if(et == TULONG || et == TLONG)
			a = ALSLL;
		break;

	case OFUNC:
		a = ACALL;
		break;

	case OASMUL:
	case OMUL:
		mul = 1;
		a = AGOK;
		if(et == TUCHAR || et == TCHAR)
			a = AMULUB;	/* same as AMULB when result 8 bit */
		break;

	case OASDIV:
	case ODIV:
		a = AGOK;
		break;

	case OASMOD:
	case OMOD:
		a = AGOK;
		break;

	case OASLMUL:
	case OLMUL:
		mul = 1;
		a = AGOK;
		if(et == TUCHAR || et == TCHAR)
			a = AMULUB;
		break;

	case OASLMOD:
	case OLMOD:
		a = AGOK;
		break;

	case OASLDIV:
	case OLDIV:
		a = AGOK;
		break;

	case OEQ:
	case ONE:
	case OLT:
	case OLE:
	case OGE:
	case OGT:
	case OLO:
	case OLS:
	case OHS:
	case OHI:
		a = ACMPW;
		if(et == TUCHAR || et == TCHAR)
			a = ACMPB;
		else if(et == TULONG || et == TLONG)
			a = ACMPL;
/*
		if(et == TFLOAT)
			a = ACMPF;
		else
		if(et == TDOUBLE || et == TVLONG)
			a = ACMPD;
*/
		nextpc();
		p->as = a;
		rr = f->op == OREGISTER && t->op == OREGISTER;
		swap = 0;
		switch(o) {
		case OEQ:
			a = ABEQ;
			break;
		case ONE:
			a = ABNE;
			break;
		case OLT:
			a = ABLT;
			break;
		case OLE:
			if(rr){
				a = ABGE;
				swap = 1;
				break;
			}
			a = ABLE;
			break;
		case OGE:
			a = ABGE;
			break;
		case OGT:
			if(rr){
				a = ABLT;
				swap = 1;
				break;
			}
			a = ABGT;
			break;
		case OLO:
			a = ABLO;
			break;
		case OLS:
			if(rr){
				a = ABSH;
				swap = 1;
				break;
			}
			a = ABSL;
			break;
		case OHS:
			a = ABSH;
			break;
		case OHI:
			if(rr){
				a = ABLO;
				swap = 1;
				break;
			}
			a = ABHI;
			break;
		}
		if(swap){
			nzaddr(f, &p->to);
			nzaddr(t, &p->from);
		}
		else{
			nzaddr(f, &p->from);
			nzaddr(t, &p->to);
		}
		f = Z;
		t = Z;
		break;
	}
	if(a == AGOK)
		diag(Z, "bad instruction gopcode %O", o);
	nextpc();
	p->as = a;
	if(f != Z)
		nzaddr(f, &p->from);
	if(t != Z)
		naddr(t, &p->to);
	if(debug['g'])
		print("%P\n", p);
	if(mul){
		nextpc();
		p->as = AMOVB;
		p->from.type = p->to.type = D_REG;
		p->from.reg = 0;
		p->to.reg = t->reg;
	}
}

/* put f in a register first */
void
gopcode2(int o, Node *f, Node *t)
{
	Node nod;

	// regalloc(&nod, t, Z, 0);
	nodreg(&nod, t, REGTMP);
	gopcode(OAS, f, &nod);
	gopcode(o, &nod, t);
	// regfree(&nod);
}

samaddr(Node *f, Node *t)
{

	if(f->op != t->op)
		return 0;
	switch(f->op) {

	case OREGISTER:
		if(f->reg != t->reg)
			break;
		return 1;
	}
	return 0;
}

void
gbranch(int o)
{
	int a;

	a = AGOK;
	switch(o) {
	case ORETURN:
		a = ARET;
		break;
	case OGOTO:
		a = ABR;
		break;
	}
	nextpc();
	if(a == AGOK) {
		diag(Z, "bad in gbranch %O",  o);
		nextpc();
	}
	p->as = a;
}

void
patch(Prog *op, long pc)
{

	op->to.offset = pc;
	op->to.type = D_BRANCH;
}

void
gpseudo(int a, Sym *s, Node *n)
{

	nextpc();
	p->as = a;
	p->from.type = D_OREG;
	p->from.sym = s;
	p->from.name = D_EXTERN;
	if(s->class == CSTATIC)
		p->from.name = D_STATIC;
	naddr(n, &p->to);
	if(a == ADATA || a == AGLOBL)
		pc--;
}

int
hireg(Node *n)
{
	return n->op == OREGISTER && n->reg >= REGHI;
}

int
sconst(Node *n)
{
	vlong vv;

	if(n->op == OCONST) {
		if(!typefd[n->type->etype]) {
			vv = n->vconst;
			if(vv >= (vlong)0 && vv < (vlong)256)
				return 1;
			return 1;
		}
	}
	return 0;
}

int
sval(long v)
{
	if(v >= -32768 && v < 32768)
		return 1;
	return 0;
}

long
exreg(Type *t)
{
	long o;

	// 0 always returned
	if(typechlp[t->etype]) {
		if(exregoffset <= NREG-1)
			return 0;
		o = exregoffset;
		exregoffset--;
		return o;
	}
	if(typefd[t->etype]) {
		if(exfregoffset <= NFREG-1)
			return 0;
		o = exfregoffset + NREG;
		exfregoffset--;
		return o;
	}
	return 0;
}

schar	ewidth[NTYPE] =
{
	-1,		/* [TXXX] */
	SZ_CHAR,	/* [TCHAR] */
	SZ_CHAR,	/* [TUCHAR] */
	SZ_SHORT,	/* [TSHORT] */
	SZ_SHORT,	/* [TUSHORT] */
	SZ_INT,		/* [TINT] */
	SZ_INT,		/* [TUINT] */
	SZ_LONG,	/* [TLONG] */
	SZ_LONG,	/* [TULONG] */
	SZ_VLONG,	/* [TVLONG] */
	SZ_VLONG,	/* [TUVLONG] */
	SZ_FLOAT,	/* [TFLOAT] */
	SZ_DOUBLE,	/* [TDOUBLE] */
	SZ_IND,		/* [TIND] */
	0,		/* [TFUNC] */
	-1,		/* [TARRAY] */
	0,		/* [TVOID] */
	-1,		/* [TSTRUCT] */
	-1,		/* [TUNION] */
	SZ_INT,		/* [TENUM] */
};

long	ncast[NTYPE] =
{
	0,				/* [TXXX] */
	BCHAR|BUCHAR,			/* [TCHAR] */
	BCHAR|BUCHAR,			/* [TUCHAR] */
	BSHORT|BUSHORT|BINT|BUINT|BIND,			/* [TSHORT] */
	BSHORT|BUSHORT|BINT|BUINT|BIND,			/* [TUSHORT] */
	BSHORT|BUSHORT|BINT|BUINT|BIND,	/* [TINT] */
	BSHORT|BUSHORT|BINT|BUINT|BIND,	/* [TUINT] */
	BLONG|BULONG,	/* [TLONG] */
	BLONG|BULONG,	/* [TULONG] */
	BVLONG|BUVLONG,			/* [TVLONG] */
	BVLONG|BUVLONG,			/* [TUVLONG] */
	BFLOAT,				/* [TFLOAT] */
	BDOUBLE,			/* [TDOUBLE] */
	BSHORT|BUSHORT|BINT|BUINT|BIND,		/* [TIND] */
	0,				/* [TFUNC] */
	0,				/* [TARRAY] */
	0,				/* [TVOID] */
	BSTRUCT,			/* [TSTRUCT] */
	BUNION,				/* [TUNION] */
	0,				/* [TENUM] */
};
