#include "gc.h"

#define X	1729
#define MAX(x, y)	((x) > (y) ? (x) : (y))

static void subptr(Node*);

static Static *statics;

Static*
lookupstatic(Sym *s)
{
	Static *st;

	for(st = statics; st != nil; st = st->next)
		if(st->sym == s)
			return st;
	st = (Static*)malloc(sizeof(Static));
	st->sym = s;
	st->class = CSTATIC;
	st->next = statics;
	statics = st;
	if(s->name == nil || s->name[0] == '.')
		st->class = CSTATIC0;
	return st;
}

static int
ckuse(Node *n, Sym *s, int i, Sym **ps)
{
	int l, r;

	if(n == Z)
		return 0;
	switch(n->op){
	default:
		l = ckuse(n->left, s, i, ps);
		r = ckuse(n->right, s, i, ps);
		return MAX(l, r);
	case OADDR:
		l = ckuse(n->left, s, i, ps);
		if(l > 0)
			return l+1;
		break;
	case OIND:
		l = ckuse(n->left, s, i, ps);
		if(l > 0)
			return l-1;
		break;
	case OAS:
		if(n->left->op == ONAME && n->left->class == CAUTO && n->right->op == OADDR && n->right->left->op == ONAME){
			if(i){
				if(n->left->sym == s && n->right->left->sym == *ps)
					return X;		/* p = &tab ok when checking p */
			}
			else{
				l = ckuse(n->left, s, i, ps);
				r = ckuse(n->right, s, i, ps);
				if(l == 0 && r == X+1 && *ps == nil){		/* allow one p = &tab */
					*ps = n->left->sym;
					return X;
				}
			}
		}
		/* FALLTHROUGH */
	case OASI:
	case OASADD:
	case OASAND:
	case OASASHL:
	case OASASHR:
	case OASDIV:
	case OASLDIV:
	case OASLMOD:
	case OASLMUL:
	case OASLSHR:
	case OASMOD:
	case OASMUL:
	case OASOR:
	case OASSUB:
	case OASXOR:
	case OFUNC:
		l = ckuse(n->left, s, i, ps);
		r = ckuse(n->right, s, i, ps);
		if(l == 0)
			return r;
		return MAX(l+1, r);
	case OPOSTDEC:
	case OPOSTINC:
	case OPREDEC:
	case OPREINC:
		l = ckuse(n->left, s, i, ps);
		if(l == 0)
			return 0;
		if(i && l == X+1)
			return X;		/* allow p++ etc where p = &tab */
		return l+1;
	case ONAME:
		if(n->sym == s)
			return X+i;
		break;
	}
	return 0;
}

static void
ckstatics(Node *n)
{
	int i, c;
	Sym *s, *sloc;
	Static *st;

	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type == T || s->type->width == 0 || s->type == types[TENUM] || s->class != CSTATIC || s->name == nil || s->name[0] == '.')
			continue;
		st = lookupstatic(s);
		if(st->class == CSTATIC){
			sloc = nil;
			c = ckuse(n, s, 0, &sloc);
			if(c > X || 0 && sloc && ckuse(n, sloc, 1, &s) > X)
				st->class = CSTATIC0;
		}
	}
}

void
codgen(Node *n, Node *nn)
{
	Prog *sp;
	Node *n1, nod, nod1;

	cursafe = 0;
	curarg = 0;
	maxargsafe = 0;

	/*
	 * isolate name
	 */
	for(n1 = nn;; n1 = n1->left) {
		if(n1 == Z) {
			diag(nn, "cant find function name");
			return;
		}
		if(n1->op == ONAME)
			break;
	}
	nearln = nn->lineno;
	gpseudo(ATEXT, n1->sym, nodconst(stkoff, Z));
	sp = p;

	/*
	 * isolate first argument
	 */
	if(REGARG >= 0) {
		if(typesuv[thisfn->link->etype]) {
			nod1 = *nodret->left;
			nodreg(&nod, &nod1, argreg(nodret->left->etype));
			gopcode(OAS, &nod, &nod1);
		} else
		if(firstarg && typechlp[firstargtype->etype]) {
			nod1 = *nodret->left;
			nod1.sym = firstarg;
			nod1.type = firstargtype;
			nod1.xoffset = align(0, firstargtype, Aarg1);
			nod1.etype = firstargtype->etype;
			nodreg(&nod, &nod1, argreg(firstargtype->etype));
			gopcode(OAS, &nod, &nod1);
		}
	}

	retok = 0;
	gen(n);
	ckstatics(n);
	if(!retok)
		if(thisfn->link->etype != TVOID)
			warn(Z, "no return at end of function: %s", n1->sym->name);
	noretval(3, thisfn->link->etype);
	gbranch(ORETURN);

	if(!debug['N'] || debug['R'] || debug['P'])
		regopt(sp);

	sp->to.offset += maxargsafe;
}

void
supgen(Node *n)
{
	long spc;
	Prog *sp;

	if(n == Z)
		return;
	suppress++;
	spc = pc;
	sp = lastp;
	gen(n);
	lastp = sp;
	pc = spc;
	sp->link = nil;
	suppress--;
}

void
gen(Node *n)
{
	Node *l, nod;
	Prog *sp, *spc, *spb;
	Case *cn;
	long sbc, scc;
	int o, f, et;

loop:
	if(n == Z)
		return;
	nearln = n->lineno;
	o = n->op;
	if(debug['G'])
		if(o != OLIST)
			print("%L %O\n", nearln, o);

	retok = 0;
	switch(o) {

	default:
		complex(n);
		cgen(n, Z);
		break;

	case OLIST:
		gen(n->left);

	rloop:
		n = n->right;
		goto loop;

	case ORETURN:
		retok = 1;
		complex(n);
		if(n->type == T)
			break;
		l = n->left;
		if(l == Z) {
			noretval(3, TVOID);
			gbranch(ORETURN);
			break;
		}
		if(typesuv[n->type->etype]) {
			sugen(l, nodret, n->type->width);
			noretval(3, TVOID);
			gbranch(ORETURN);
			break;
		}
		regret(&nod, n);
		cgen(l, &nod);
		regfree(&nod);
		if(typefd[n->type->etype])
			noretval(1, n->type->etype);
		else
			noretval(2, n->type->etype);
		gbranch(ORETURN);
		break;

	case OLABEL:
		l = n->left;
		if(l) {
			l->pc = pc;
			if(l->label)
				patch(l->label, pc);
		}
		gbranch(OGOTO);	/* prevent self reference in reg */
		patch(p, pc);
		goto rloop;

	case OGOTO:
		retok = 1;
		n = n->left;
		if(n == Z)
			return;
		if(n->complex == 0) {
			diag(Z, "label undefined: %s", n->sym->name);
			return;
		}
		if(suppress)
			return;
		gbranch(OGOTO);
		if(n->pc) {
			patch(p, n->pc);
			return;
		}
		if(n->label)
			patch(n->label, pc-1);
		n->label = p;
		return;

	case OCASE:
		l = n->left;
		if(cases == C)
			diag(n, "case/default outside a switch");
		if(l == Z) {
			cas();
			cases->val = 0;
			cases->def = 1;
			cases->label = pc;
			goto rloop;
		}
		complex(l);
		if(l->type == T)
			goto rloop;
		if(l->op == OCONST)
		if(typechl[l->type->etype]) {
			cas();
			cases->val = l->vconst;
			cases->def = 0;
			cases->label = pc;
			goto rloop;
		}
		diag(n, "case expression must be integer constant");
		goto rloop;

	case OSWITCH:
		l = n->left;
		complex(l);
		if(l->type == T)
			break;
		et = l->type->etype;
		if(!typechl[et]) {
			diag(n, "switch expression must be integer");
			break;
		}

		gbranch(OGOTO);		/* entry */
		sp = p;

		cn = cases;
		cases = C;
		cas();

		sbc = breakpc;
		breakpc = pc;
		gbranch(OGOTO);
		spb = p;

		gen(n->right);
		gbranch(OGOTO);
		patch(p, breakpc);

		patch(sp, pc);
		regalloc(&nod, l, Z, 0);
		if(et == TCHAR || et == TUCHAR)
			et = TCHAR;
		else if(et == TLONG || et == TULONG)
			et = TLONG;
		else
			et = TINT;
		nod.type = types[et];
		cgen(l, &nod);
		doswit(&nod);
		regfree(&nod);
		patch(spb, pc);

		cases = cn;
		breakpc = sbc;
		break;

	case OWHILE:
	case ODWHILE:
		l = n->left;
		gbranch(OGOTO);		/* entry */
		sp = p;

		scc = continpc;
		continpc = pc;
		gbranch(OGOTO);
		spc = p;

		sbc = breakpc;
		breakpc = pc;
		gbranch(OGOTO);
		spb = p;

		patch(spc, pc);
		if(n->op == OWHILE)
			patch(sp, pc);
		bcomplex(l, Z);		/* test */
		patch(p, breakpc);

		if(n->op == ODWHILE)
			patch(sp, pc);
		gen(n->right);		/* body */
		gbranch(OGOTO);
		patch(p, continpc);

		patch(spb, pc);
		continpc = scc;
		breakpc = sbc;
		break;

	case OFOR:
		l = n->left;
		gen(l->right->left);	/* init */
		gbranch(OGOTO);		/* entry */
		sp = p;

		scc = continpc;
		continpc = pc;
		gbranch(OGOTO);
		spc = p;

		sbc = breakpc;
		breakpc = pc;
		gbranch(OGOTO);
		spb = p;

		patch(spc, pc);
		gen(l->right->right);	/* inc */
		patch(sp, pc);	
		if(l->left != Z) {	/* test */
			bcomplex(l->left, Z);
			patch(p, breakpc);
		}
		gen(n->right);		/* body */
		gbranch(OGOTO);
		patch(p, continpc);

		patch(spb, pc);
		continpc = scc;
		breakpc = sbc;
		break;

	case OCONTINUE:
		if(continpc < 0) {
			diag(n, "continue not in a loop");
			break;
		}
		gbranch(OGOTO);
		patch(p, continpc);
		break;

	case OBREAK:
		if(breakpc < 0) {
			diag(n, "break not in a loop");
			break;
		}
		gbranch(OGOTO);
		patch(p, breakpc);
		break;

	case OIF:
		l = n->left;
		if(bcomplex(l, n->right)) {
			if(typefd[l->type->etype])
				f = !l->fconst;
			else
				f = !l->vconst;
			if(debug['c'])
				print("%L const if %s\n", nearln, f ? "false" : "true");
			if(f) {
				supgen(n->right->left);
				gen(n->right->right);
			}
			else {
				gen(n->right->left);
				supgen(n->right->right);
			}
		}
		else {
			sp = p;
			if(n->right->left != Z)
				gen(n->right->left);
			if(n->right->right != Z) {
				gbranch(OGOTO);
				patch(sp, pc);
				sp = p;
				gen(n->right->right);
			}
			patch(sp, pc);
		}
		break;

	case OSET:
	case OUSED:
		usedset(n->left, o);
		break;
	}
}

void
usedset(Node *n, int o)
{
	if(n->op == OLIST) {
		usedset(n->left, o);
		usedset(n->right, o);
		return;
	}
	complex(n);
	switch(n->op) {
	case OADDR:	/* volatile */
		gins(ANOOP, n, Z);
		break;
	case ONAME:
		if(o == OSET)
			gins(ANOOP, Z, n);
		else
			gins(ANOOP, n, Z);
		break;
	}
}

void
noretval(int n, int et)
{

	if(n & 1) {
		gins(ANOOP, Z, Z);
		p->to.type = D_REG;
		p->to.reg = retreg(et);
	}
	if(n & 2) {
		gins(ANOOP, Z, Z);
		p->to.type = D_FREG;
		p->to.reg = FREGRET;
	}
}

static void
cktype(Node *n)
{
	int et;
	Type *t;

	t = n->type;
	if(t == T)
		return;
	et = t->etype;
	if(et == TFLOAT || et == TDOUBLE)
		diag(n, "floating point operations not implemented");
	else if(et == TVLONG)
		diag(n, "long long operations not implemented");
}

/*
 *	calculate addressability as follows
 *		CONST ==> 20		$value
 *		NAME ==> 10		name
 *		REGISTER ==> 11		register
 *		INDREG ==> 12		*[(reg)+offset]
 *		&10 ==> 2		$name
 *		ADD(2, 20) ==> 2	$name+offset
 *		ADD(3, 20) ==> 3	$(reg)+offset
 *		&12 ==> 3		$(reg)+offset
 *		*11 ==> 11		??
 *		*2 ==> 10		name
 *		*3 ==> 12		*(reg)+offset
 *	calculate complexity (number of registers)
 */
void
xcom(Node *n)
{
	Node *l, *r;
	int t;

	if(n == Z)
		return;
	if(n->op == OCAST)
		subptr(n);
	if(unarith(n))
		return;
	cktype(n);
	l = n->left;
	r = n->right;
	n->addable = 0;
	n->complex = 0;
	switch(n->op) {
	case OCONST:
		n->addable = 20;
		return;

	case OREGISTER:
		n->addable = 11;
		return;

	case OINDREG:
		n->addable = 12;
		return;

	case ONAME:
		n->addable = 10;
		return;

	case OADDR:
		xcom(l);
		if(l->addable == 10)
			n->addable = 2;
		if(l->addable == 12)
			n->addable = 3;
		break;

	case OIND:
		xcom(l);
		if(l->addable == 11)
			n->addable = 12;
		if(l->addable == 3)
			n->addable = 12;
		if(l->addable == 2)
			n->addable = 10;
		break;

	case OADD:
		xcom(l);
		xcom(r);
		if(l->addable == 20) {
			if(r->addable == 2)
				n->addable = 2;
			if(r->addable == 3)
				n->addable = 3;
		}
		if(r->addable == 20) {
			if(l->addable == 2)
				n->addable = 2;
			if(l->addable == 3)
				n->addable = 3;
		}
		break;

	case OSUB:
		xcom(l);
		xcom(r);
		if(typefd[n->type->etype] || typev[n->type->etype])
			break;
		if(vconst(l) == 0){
			n->op = ONEG;
			n->left = r;
			n->right = Z;
		}
		break;

	case OXOR:
		xcom(l);
		xcom(r);
		if(vconst(l) == -1){
			n->op = OCOM;
			n->left = r;
			n->right = Z;
		}
		break;

	case OASHL:
	case OASHR:
	case OLSHR:
	case OASASHL:
	case OASASHR:
	case OASLSHR:
		xcom(l);
		xcom(r);
		if(r->op == OCONST && r->vconst < 0){
			r->vconst = -r->vconst;
			switch(n->op){
			case OASHL:	n->op = OASHR; break;
			case OASHR:	n->op = OASHL; break;
			case OLSHR:	n->op = OASHL; break;
			case OASASHL:	n->op = OASASHR; break;
			case OASASHR:	n->op = OASASHL; break;
			case OASLSHR:	n->op = OASASHL; break;
			}
		}
		if(r->op == OCONST){
			if(n->type != r->type)
			if(convvtox(r->vconst, n->type->etype) == r->vconst)
				r->type = n->type;
		}
		break;
	
	case OASLMUL:
	case OASMUL:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASASHL;
			r->vconst = t;
			r->type = l->type;
		}
		break;

	case OMUL:
	case OLMUL:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASHL;
			r->vconst = t;
			r->type = l->type;
		}
		t = vlog(l);
		if(t >= 0) {
			n->op = OASHL;
			n->left = r;
			n->right = l;
			r = l;
			l = n->left;
			r->vconst = t;
			r->type = l->type;
		}
		break;

	case OASLDIV:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASLSHR;
			r->vconst = t;
			r->type = l->type;
		}
		break;

	case OLDIV:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OLSHR;
			r->vconst = t;
			r->type = l->type;
		}
		break;

	case OASLMOD:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OASAND;
			r->vconst--;
		}
		break;

	case OLMOD:
		xcom(l);
		xcom(r);
		t = vlog(r);
		if(t >= 0) {
			n->op = OAND;
			r->vconst--;
		}
		break;

	default:
		if(l != Z)
			xcom(l);
		if(r != Z)
			xcom(r);
		break;
	}
	if(n->addable >= 10)
		return;

	if(l != Z)
		n->complex = l->complex;
	if(r != Z) {
		if(r->complex == n->complex)
			n->complex = r->complex+1;
		else
		if(r->complex > n->complex)
			n->complex = r->complex;
	}
	if(n->complex == 0)
		n->complex++;

	if(com64(n))
		return;
	if(commul(n))
		return;

	switch(n->op) {
	case OFUNC:
		n->complex = FNX;
		break;

	case OEQ:
	case ONE:
	case OLE:
	case OLT:
	case OGE:
	case OGT:
	case OHI:
	case OHS:
	case OLO:
	case OLS:
		/*
		 * immediate operators, make const on right
		 */
		if(l->op == OCONST) {
			n->left = r;
			n->right = l;
			n->op = invrel[relindex(n->op)];
		}
		break;

	case OADD:
	case OXOR:
	case OAND:
	case OOR:
		/*
		 * immediate operators, make const on right
		 */
		if(l->op == OCONST) {
			n->left = r;
			n->right = l;
		}
		break;
	}
}

int
bcomplex(Node *n, Node *c)
{

	complex(n);
	if(n->type != T)
	if(tcompat(n, T, n->type, tnot))
		n->type = T;
	if(n->type != T) {
		if(c != Z && n->op == OCONST && deadheads(c))
			return 1;
		bool64(n);
		boolgen(n, 1, Z);
	} else
		gbranch(OGOTO);
	return 0;
}

static Node *nodmul[TULONG+1];
static Node *noddiv[TULONG+1];
static Node *nodmod[TULONG+1];
static Node *nodlsl[TULONG+1];
static Node *nodlsr[TULONG+1];
static Node *nodasr[TULONG+1];

extern Node* fvn(char*, int);

void
commulinit(void)
{
	// nodmul[TCHAR] = fvn("_mulb", TCHAR);
	// nodmul[TUCHAR] = fvn("_mulbu", TUCHAR);
	nodmul[TSHORT] = fvn("_mulw", TSHORT);
	nodmul[TUSHORT] = fvn("_mulwu", TUSHORT);
	nodmul[TINT] = fvn("_mulw", TINT);
	nodmul[TUINT] = fvn("_mulwu", TUINT);
	nodmul[TLONG] = fvn("_mull", TLONG);
	nodmul[TULONG] = fvn("_mullu", TULONG);

	noddiv[TCHAR] = fvn("_divb", TCHAR);
	noddiv[TUCHAR] = fvn("_divbu", TUCHAR);
	noddiv[TSHORT] = fvn("_divw", TSHORT);
	noddiv[TUSHORT] = fvn("_divwu", TUSHORT);
	noddiv[TINT] = fvn("_divw", TINT);
	noddiv[TUINT] = fvn("_divwu", TUINT);
	noddiv[TLONG] = fvn("_divl", TLONG);
	noddiv[TULONG] = fvn("_divlu", TULONG);

	nodmod[TCHAR] = fvn("_modb", TCHAR);
	nodmod[TUCHAR] = fvn("_modbu", TUCHAR);
	nodmod[TSHORT] = fvn("_modw", TSHORT);
	nodmod[TUSHORT] = fvn("_modwu", TUSHORT);
	nodmod[TINT] = fvn("_modw", TINT);
	nodmod[TUINT] = fvn("_modwu", TUINT);
	nodmod[TLONG] = fvn("_modl", TLONG);
	nodmod[TULONG] = fvn("_modlu", TULONG);

	nodlsl[TCHAR] = fvn("_lslb", TCHAR);
	nodlsl[TUCHAR] = fvn("_lslb", TUCHAR);
	nodlsl[TSHORT] = fvn("_lslw", TSHORT);
	nodlsl[TUSHORT] = fvn("_lslw", TUSHORT);
	nodlsl[TINT] = fvn("_lslw", TINT);
	nodlsl[TUINT] = fvn("_lslw", TUINT);
	nodlsl[TLONG] = fvn("_lsll", TLONG);
	nodlsl[TULONG] = fvn("_lsll", TULONG);

	nodlsr[TCHAR] = fvn("_lsrb", TCHAR);
	nodlsr[TUCHAR] = fvn("_lsrb", TUCHAR);
	nodlsr[TSHORT] = fvn("_lsrw", TSHORT);
	nodlsr[TUSHORT] = fvn("_lsrw", TUSHORT);
	nodlsr[TINT] = fvn("_lsrw", TINT);
	nodlsr[TUINT] = fvn("_lsrw", TUINT);
	nodlsr[TLONG] = fvn("_lsrl", TLONG);
	nodlsr[TULONG] = fvn("_lsrl", TULONG);

	nodasr[TCHAR] = fvn("_asrb", TCHAR);
	nodasr[TUCHAR] = fvn("_asrb", TUCHAR);
	nodasr[TSHORT] = fvn("_asrw", TSHORT);
	nodasr[TUSHORT] = fvn("_asrw", TUSHORT);
	nodasr[TINT] = fvn("_asrw", TINT);
	nodasr[TUINT] = fvn("_asrw", TUINT);
	nodasr[TLONG] = fvn("_asrl", TLONG);
	nodasr[TULONG] = fvn("_asrl", TULONG);
}

static int
cando(int a, int et, long s)
{
	switch(ewidth[et]){
	case 1:
		return 1;
	case 2:
		if(a == OASHL && s < 8)
			return 1;
		if(a == OASHR && s == 7)
			return 1;
		return s == 0 || s == 1 || s == 2 || s == 8 || s == 10 || s >= 16;
	case 4:
		if(a == OLSHR && (s == 3 || s == 4))
			return 1;
		return s == 0 || s == 1 || s == 2 || s == 8 || s == 10 || s == 16 || s == 24 || s >= 32;
	}
	return 0;
}

int
commul(Node *n)
{
	int a, et, o;
	Node *l, *r, *nn, **pf, *t;

	if(n->type == T)
		return 0;
	et = n->type->etype;
	if(et == TIND)
		et = TINT;
	if(et == TXXX || et > TULONG)
		return 0;
	a = 0;
	o = n->op;
	switch(o){
	case OASMUL:
	case OASLMUL:
		a = 1;
	case OMUL:
	case OLMUL:
		if((o == OMUL || o == OLMUL) && mulcon(n, Z, 1))
			return 0;
		if(et == TCHAR || et == TUCHAR)
			return 0;
		pf = nodmul;
		break;
	case OASDIV:
	case OASLDIV:
		a = 1;
	case ODIV:
	case OLDIV:
		pf = noddiv;
		break;
	case OASMOD:
	case OASLMOD:
		a = 1;
	case OMOD:
	case OLMOD:
		pf = nodmod;
		break;
	case OASASHL:
		a = 1;
	case OASHL:
		if(n->right->op == OCONST && cando(OASHL, et, n->right->vconst))
			return 0;
		pf = nodlsl;
		break;
	case OASASHR:
		a = 1;
	case OASHR:
		if(n->right->op == OCONST && cando(OASHR, et, n->right->vconst))
			return 0;
		pf = nodasr;
		break;
	case OASLSHR:
		a = 1;
	case OLSHR:
		if(n->right->op == OCONST && cando(OLSHR, et, n->right->vconst))
			return 0;
		pf = nodlsr;
		break;
	default:
		return 0;
	}
	l = n->left;
	r = n->right;
	nn = n;
	if(a){
		if(side(l)){
			t = new(OXXX, Z, Z);
			regsalloc(t, l);
			n->left = new(OAS, t, new(OADDR, l, Z));
			n->left->type = n->left->right->type = t->type = typ(TIND, n->type);
			n->right = new(OXXX, new(OIND, t, Z), r);
			n->right->type = n->right->left->type = n->type;
			n->op = OCOMMA;
			n = n->right;
			l = n->left;
		}
		n->right = new(OXXX, l, r);
		n->right->type = n->type;
		n->op = OAS;
		n = n->right;
	}
	n->left = pf[et];
	n->right = new(OLIST, l, r);
	n->op = OFUNC;
	xcom(nn);
	return 1;
}

static int
canremcast(Node *n, int f, int t)
{
	int et;
	vlong v;
	Node *l, *r;

	if(n == Z)
		return 1;
	l = n->left;
	r = n->right;
	et = n->type->etype;
	if(et == TSHORT || et == TUSHORT)
		et += 2;
	if(et != t && !(f == TUCHAR && t == TUINT && et == TINT))
		return 0;
	switch(n->op){
	case OEQ:
	case ONE:
	case OLT:
	case OGE:
	case OGT:
	case OLE:
	case OLS:
	case OHS:
	case OLO:
	case OHI:
	case OADD:
	case OSUB:
	case OMUL:
	case OLMUL:
	case ODIV:
	case OLDIV:
	case OMOD:
	case OLMOD:
	case OAND:
	case OOR:
	case OXOR:
	case OLSHR:
	case OASHL:
	case OASHR:
	case OANDAND:
	case OOROR:
	case OASADD:
	case OASSUB:
	case OASMUL:
	case OASLMUL:
	case OASDIV:
	case OASLDIV:
	case OASMOD:
	case OASLMOD:
	case OASAND:
	case OASOR:
	case OASXOR:
	case OASLSHR:
	case OASASHL:
	case OASASHR:
	case OAS:
	case OLIST:
		return canremcast(l, f, t) && canremcast(r, f, t);
	case OPOS:
	case ONEG:
	case OCOM:
	case ONOT:
	case OPOSTDEC:
	case OPOSTINC:
	case OPREDEC:
	case OPREINC:
		return canremcast(l, f, t);
	case OCOMMA:
	case OCOND:
		return canremcast(r, f, t);
	case OCAST:
		return canremcast(l, f, f);
	case OCONST:
		v = convvtox(n->vconst, f);
		return v == n->vconst;
	case ONAME:
	case OIND:
	case OFUNC:
		return t == TCHAR || t == TUCHAR;
	}
	return 0;
}

static Node*
remcast(Node *n, int f, int t)
{
	vlong v;
	Node *l, *r;

	if(n == Z)
		return Z;
	l = n->left;
	r = n->right;
	switch(n->op){
	case OEQ:
	case ONE:
	case OLT:
	case OGE:
	case OGT:
	case OLE:
	case OLS:
	case OHS:
	case OLO:
	case OHI:
	case OADD:
	case OSUB:
	case OMUL:
	case OLMUL:
	case ODIV:
	case OLDIV:
	case OMOD:
	case OLMOD:
	case OAND:
	case OOR:
	case OXOR:
	case OLSHR:
	case OASHL:
	case OASHR:
	case OANDAND:
	case OOROR:
	case OASADD:
	case OASSUB:
	case OASMUL:
	case OASLMUL:
	case OASDIV:
	case OASLDIV:
	case OASMOD:
	case OASLMOD:
	case OASAND:
	case OASOR:
	case OASXOR:
	case OASLSHR:
	case OASASHL:
	case OASASHR:
	case OAS:
	case OLIST:
		n->left = remcast(l, f, t);
		n->right = remcast(r, f, t);
		n->type = types[f];
		break;
	case OPOS:
	case ONEG:
	case OCOM:
	case ONOT:
	case OPOSTDEC:
	case OPOSTINC:
	case OPREDEC:
	case OPREINC:
		n->left = remcast(l, f, t);
		n->type = types[f];
		break;
	case OCOMMA:
	case OCOND:
		n->right = remcast(r, f, t);
		break;
	case OCAST:
		n = remcast(l, f, f);
		n->type = types[f];
		break;
	case OCONST:
		v = convvtox(n->vconst, f);
		if(v != n->vconst)
			diag(n, "bad con in remcast");
		n->type = types[f];
		break;
	case ONAME:
	case OIND:
	case OFUNC:
		if(t != TCHAR && t != TUCHAR)
			diag(n, "bad char type in remcast");
		break;
	}
	return n;
}

/* make subtraction of pointers an int type */
static void
subptr(Node *n)
{
	int et;
	Node *l, *r;

	if(n == Z || n->type == T)
		return;
	l = n->left;
	r = n->right;
	et = n->type->etype;
	switch(n->op){
	case OSUB:
		if(et == TLONG && l->type->etype == TIND && r->type->etype == TIND)
			n->type = types[TINT];
		break;
	case OASHR:
	case ODIV:
		if(et == TLONG && l->op == OSUB && l->type->etype == TIND)
			n->type = types[TINT];
		break;
	case OCAST:
		if(et == TINT){
			subptr(l);
			if(l->type->etype == TINT)
				*n = *l;
		}
		break;
	}
}

int
unarith(Node *n)
{
	int et;
	Node *l, *r;

	if(n == Z || n->type == T)
		return 0;
	l = n->left;
	r = n->right;
	et = n->type->etype;
	switch(n->op){
	case OCAST:
		if(et != TCHAR && et != TUCHAR)
			return 0;
		if(canremcast(l, et, et+4)){
			l = remcast(l, et, et+4);
			*n = *l;
			xcom(n);
			return 1;
		}
		break;
	case OEQ:
	case ONE:
	case OLT:
	case OGE:
	case OGT:
	case OLE:
	case OLS:
	case OHS:
	case OLO:
	case OHI:
		et = n->left->type->etype;
		if(et == TSHORT || et == TSHORT)
			et += 2;
		if(et != TINT && et != TUINT)
			return 0;
		if(canremcast(l, et-4, et) && canremcast(r, et-4, et)){
			n->left = remcast(l, et-4, et);
			n->right = remcast(r, et-4, et);
			xcom(n);
			return 1;
		}
		break;
	}
	return 0;
}
