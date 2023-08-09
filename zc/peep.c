#include "gc.h"

void
peep(void)
{
	Reg *r, *r1, *r2;
	Prog *p, *p1;
	int t;
/*
 * complete R structure
 */
	t = 0;
	for(r=firstr; r!=R; r=r1) {
		r1 = r->link;
		if(r1 == R)
			break;
		p = r->prog->link;
		while(p != r1->prog)
		switch(p->as) {
		default:
			r2 = rega();
			r->link = r2;
			r2->link = r1;

			r2->prog = p;
			r2->p1 = r;
			r->s1 = r2;
			r2->s1 = r1;
			r1->p1 = r2;

			r = r2;
			t++;

		case ADATA:
		case AGLOBL:
		case ANAME:
		case ASIGNAME:
			p = p->link;
		}
	}

loop1:
	t = 0;
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		// if(p->as == AMOVL || p->as == AMOVF || p->as == AMOVD)
		if(p->as == AMOVL || p->as == AMOVW || p->as == AMOVB)
		if(regtyp(&p->to)) {
			if(p->from.type == D_CONST)
				constprop(&p->from, &p->to, r->s1);
			else if(regtyp(&p->from))
			if(p->from.type == p->to.type) {
				if(copyprop(r)) {
					excise(r);
					t++;
				} else
				if(subprop(r) && copyprop(r)) {
					excise(r);
					t++;
				}
			}
		}
	}
	if(t)
		goto loop1;
	/*
	 * look for MOVB x,R; MOVB R,R
	 */
	for(r=firstr; r!=R; r=r->link) {
		p = r->prog;
		switch(p->as) {
		default:
			continue;
		case AMOVB:
		case AMOVBW:
		case AMOVBL:
		case AMOVBZW:
		case AMOVBZL:
		case AMOVW:
		case AMOVWL:
		case AMOVWZL:
		case AMOVL:
			if(p->to.type != D_REG)
				continue;
			break;
		}
		r1 = r->link;
		if(r1 == R)
			continue;
		p1 = r1->prog;
		if(p1->as != p->as)
			continue;
		if(p1->from.type != D_REG || p1->from.reg != p->to.reg)
			continue;
		if(p1->to.type != D_REG || p1->to.reg != p->to.reg)
			continue;
		excise(r1);
	}
}

void
excise(Reg *r)
{
	Prog *p;

	p = r->prog;
	p->as = ANOOP;
	p->from = zprog.from;
	p->to = zprog.to;
}

Reg*
uniqp(Reg *r)
{
	Reg *r1;

	r1 = r->p1;
	if(r1 == R) {
		r1 = r->p2;
		if(r1 == R || r1->p2link != R)
			return R;
	} else
		if(r->p2 != R)
			return R;
	return r1;
}

Reg*
uniqs(Reg *r)
{
	Reg *r1;

	r1 = r->s1;
	if(r1 == R) {
		r1 = r->s2;
		if(r1 == R)
			return R;
	} else
		if(r->s2 != R)
			return R;
	return r1;
}

int
regtyp(Adr *a)
{

	if(a->type == D_REG)
		return 1;
	if(a->type == D_FREG)
		return 1;
	return 0;
}

/*
 * the idea is to substitute
 * one register for another
 * from one MOV to another
 *	MOV	a, R0
 *	ADD	b, R0	/ no use of R1
 *	MOV	R0, R1
 * would be converted to
 *	MOV	a, R1
 *	ADD	b, R1
 *	MOV	R1, R0
 * hopefully, then the former or latter MOV
 * will be eliminated by copy propagation.
 */
int
subprop(Reg *r0)
{
	Prog *p;
	Adr *v1, *v2;
	Reg *r;
	int t;

	p = r0->prog;
	v1 = &p->from;
	if(!regtyp(v1))
		return 0;
	v2 = &p->to;
	if(!regtyp(v2))
		return 0;
	for(r=uniqp(r0); r!=R; r=uniqp(r)) {
		if(uniqs(r) == R)
			break;
		p = r->prog;
		switch(p->as) {
		case ACALL:
			return 0;

		case AADDB:
		case AADDW:
		case AADDL:
		case ASUBB:
		case ASUBW:
		case ASUBL:
		case AANDB:
		case AANDW:
		case AANDL:
		case AORB:
		case AORW:
		case AORL:
		case AEORB:
		case AEORW:
		case AEORL:
		case AASRB:
		case AASRW:
		case AASRL:
		case ALSLB:
		case ALSLW:
		case ALSLL:
		case ALSRB:
		case ALSRW:
		case ALSRL:
		case ACLRB:
		case ACLRW:
		case ACLRL:
		case ACMPB:
		case ACMPW:
		case ACMPL:
		case ACOMB:
		case ACOMW:
		case ACOML:
		case AMULB:
		case AMULUB:
		case ANEGB:
		case ANEGW:
		case ANEGL:
			return 0;

		// case AMOVF:
		// case AMOVD:
		case AMOVB:
		case AMOVW:
		case AMOVL:
			if(p->to.type == v1->type)
			if(p->to.reg == v1->reg)
				goto gotit;
			break;

		}
		if(copyau(&p->from, v2) ||
		   copyau(&p->to, v2))
			break;
		if(copysub(&p->from, v1, v2, 0) ||
		   copysub(&p->to, v1, v2, 0))
			break;
	}
	return 0;

gotit:
	copysub(&p->to, v1, v2, 1);
	if(debug['P']) {
		print("gotit: %D->%D\n%P", v1, v2, r->prog);
		if(p->from.type == v2->type)
			print(" excise");
		print("\n");
	}
	for(r=uniqs(r); r!=r0; r=uniqs(r)) {
		p = r->prog;
		copysub(&p->from, v1, v2, 1);
		copysub(&p->to, v1, v2, 1);
		if(debug['P'])
			print("%P\n", r->prog);
	}
	t = v1->reg;
	v1->reg = v2->reg;
	v2->reg = t;
	if(debug['P'])
		print("%P last\n", r->prog);
	return 1;
}

/*
 * The idea is to remove redundant copies.
 *	v1->v2	F=0
 *	(use v2	s/v2/v1/)*
 *	set v1	F=1
 *	use v2	return fail
 *	-----------------
 *	v1->v2	F=0
 *	(use v2	s/v2/v1/)*
 *	set v1	F=1
 *	set v2	return success
 */
int
copyprop(Reg *r0)
{
	Prog *p;
	Adr *v1, *v2;
	Reg *r;

	p = r0->prog;
	v1 = &p->from;
	v2 = &p->to;
	if(copyas(v1, v2))
		return 1;
	for(r=firstr; r!=R; r=r->link)
		r->active = 0;
	return copy1(v1, v2, r0->s1, 0);
}

int
copy1(Adr *v1, Adr *v2, Reg *r, int f)
{
	int t;
	Prog *p;

	if(r->active) {
		if(debug['P'])
			print("act set; return 1\n");
		return 1;
	}
	r->active = 1;
	if(debug['P'])
		print("copy %D->%D f=%d\n", v1, v2, f);
	for(; r != R; r = r->s1) {
		p = r->prog;
		if(debug['P'])
			print("%P", p);
		if(!f && uniqp(r) == R) {
			f = 1;
			if(debug['P'])
				print("; merge; f=%d", f);
		}
		t = copyu(p, v2, A);
		switch(t) {
		case 2:	/* rar, cant split */
			if(debug['P'])
				print("; %Drar; return 0\n", v2);
			return 0;

		case 3:	/* set */
			if(debug['P'])
				print("; %Dset; return 1\n", v2);
			return 1;

		case 1:	/* used, substitute */
		case 4:	/* use and set */
			if(f) {
				if(!debug['P'])
					return 0;
				if(t == 4)
					print("; %Dused+set and f=%d; return 0\n", v2, f);
				else
					print("; %Dused and f=%d; return 0\n", v2, f);
				return 0;
			}
			if(copyu(p, v2, v1)) {
				if(debug['P'])
					print("; sub fail; return 0\n");
				return 0;
			}
			if(debug['P'])
				print("; sub%D/%D", v2, v1);
			if(t == 4) {
				if(debug['P'])
					print("; %Dused+set; return 1\n", v2);
				return 1;
			}
			break;
		}
		if(!f) {
			t = copyu(p, v1, A);
			if(!f && (t == 2 || t == 3 || t == 4)) {
				f = 1;
				if(debug['P'])
					print("; %Dset and !f; f=%d", v1, f);
			}
		}
		if(debug['P'])
			print("\n");
		if(r->s2)
			if(!copy1(v1, v2, r->s2, f))
				return 0;
	}
	return 1;
}

static int
shift(int a)
{
	switch(a){
	case ALSLB:
	case ALSLW:
	case ALSLL:
	case ALSRB:
	case ALSRW:
	case ALSRL:
	case AASRB:
	case AASRW:
	case AASRL:
	case AROLB:
	case AROLW:
	case AROLL:
	case ARORB:
	case ARORW:
	case ARORL:
		return 1;
	}
	return 0;
}

/*
 * The idea is to remove redundant constants.
 *	$c1->v1
 *	($c1->v2 s/$c1/v1)*
 *	set v1  return
 * The v1->v2 should be eliminated by copy propagation.
 */
void
constprop(Adr *c1, Adr *v1, Reg *r)
{
	Prog *p;

	if(debug['C'])
		print("constprop %D->%D\n", c1, v1);
	for(; r != R; r = r->s1) {
		p = r->prog;
		if(debug['C'])
			print("%P", p);
		if(uniqp(r) == R) {
			if(debug['C'])
				print("; merge; return\n");
			return;
		}
		if((p->as == AMOVL || p->as == AMOVW || p->as == AMOVB) && copyas(&p->from, c1)) {
				if(debug['C'])
					print("; sub%D/%D", &p->from, v1);
				p->from = *v1;
		} else if(!shift(p->as) && copyu(p, v1, A) > 1) {
			if(debug['C'])
				print("; %Dset; return\n", v1);
			return;
		}
		if(debug['C'])
			print("\n");
		if(r->s2)
			constprop(c1, v1, r->s2);
	}
}

/*
 * return
 * 1 if v only used (and substitute),
 * 2 if read-alter-rewrite
 * 3 if set
 * 4 if set and used
 * 0 otherwise (not touched)
 */
int
copyu(Prog *p, Adr *v, Adr *s)
{

	switch(p->as) {

	default:
		if(debug['P'])
			print(" (???)");
		return 2;
		
	case ANOOP:	/* read, write */
	// case AMOVF:
	// case AMOVD:
	case AMOVB:
	case AMOVBW:
	case AMOVBL:
	case AMOVBZW:
	case AMOVBZL:
	case AMOVW:
	case AMOVWL:
	case AMOVWZL:
	case AMOVL:
	// case AMOVDW:
	// case AMOVWD:
	// case AMOVFD:
	// case AMOVDF:
		if(s != A) {
			if(copysub(&p->from, v, s, 1))
				return 1;
			if(!copyas(&p->to, v))
				if(copysub(&p->to, v, s, 1))
					return 1;
			return 0;
		}
		if(copyas(&p->to, v)) {
			if(copyau(&p->from, v))
				return 4;
			return 3;
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		return 0;

	case ALSLB:
	case ALSLW:
	case ALSLL:
	case ALSRB:
	case ALSRW:
	case ALSRL:
	case AASRB:
	case AASRW:
	case AASRL:
	case AORB:
	case AORW:
	case AORL:
	case AANDB:
	case AANDW:
	case AANDL:
	case AEORB:
	case AEORW:
	case AEORL:
	case AADDB:
	case AADDW:
	case AADDL:
	case ASUBB:
	case ASUBW:
	case ASUBL:
	case AMULB:
	case AMULUB:
	// case AADDF:
	// case AADDD:
	// case ASUBF:
	// case ASUBD:
	// case AMULF:
	// case AMULD:
	// case ADIVF:
	// case ADIVD:
	// case ACMPF:
	// case ACMPD:
	case ACLRB:
	case ACLRW:
	case ACLRL:
	case ACOMB:
	case ACOMW:
	case ACOML:
	case ANEGB:
	case ANEGW:
	case ANEGL:
		if(copyas(&p->to, v))
			return 2;
		if(s != A) {
			if(copysub(&p->from, v, s, 1))
				return 1;
			if(!copyas(&p->to, v))
				if(copysub(&p->to, v, s, 1))
					return 1;
			return 0;
		}
		if(copyas(&p->to, v)) {
			if(copyau(&p->from, v))
				return 4;
			return 3;
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		return 0;

	case ABEQ:	/* no reference */
	case ABNE:
	case ABCS:
	case ABCC:
	case ABSH:
	case ABLO:
	case ABMI:
	case ABPL:
	case ABVS:
	case ABVC:
	case ABHI:
	case ABSL:
	case ABGE:
	case ABLT:
	case ABGT:
	case ABLE:
		break;

	case ACMPB:	/* read, read */
	case ACMPW:
	case ACMPL:
		if(s != A) {
			if(copysub(&p->from, v, s, 1))
				return 1;
			return copysub(&p->to, v, s, 1);
		}
		if(copyau(&p->from, v))
			return 1;
		if(copyau(&p->to, v))
			return 1;
		return 0;

	case ABR:	/* funny */
		if(s != A) {
			if(copysub(&p->to, v, s, 1))
				return 1;
			return 0;
		}
		if(copyau(&p->to, v))
			return 1;
		return 0;

	case ARET:	/* funny */
	case ARETI:
		if(v->type == D_REG){
			int r;

			r = retreg(v->etype);
			if(v->reg == r)
				return 2;
		}
		if(v->type == D_FREG)
		if(v->reg == FREGRET)
			return 2;

	case ACALL:	/* funny */
		if(v->type == D_REG) {
			if(v->reg <= REGEXT && v->reg > exregoffset)
				return 2;
			if(v->reg == argreg(v->etype))
				return 2;
		}
		if(v->type == D_FREG)
			if(v->reg <= FREGEXT && v->reg > exfregoffset)
				return 2;

		if(s != A) {
			if(copysub(&p->to, v, s, 1))
				return 1;
			return 0;
		}
		if(copyau(&p->to, v))
			return 4;
		return 3;

	case ATEXT:	/* funny */
		if(v->type == D_REG)
			if(v->reg == argreg(v->etype))
				return 3;
		return 0;
	}
	return 0;
}

/*
 * direct reference,
 * could be set/use depending on
 * semantics
 */
int
copyas(Adr *a, Adr *v)
{

	if(regtyp(v)) {
		if(a->type == v->type)
		if(a->reg == v->reg)
			return 1;
	} else if(v->type == D_CONST) {		/* for constprop */
		if(a->type == v->type)
		if(a->name == v->name)
		if(a->sym == v->sym)
		if(a->reg == v->reg)
		if(a->offset == v->offset)
			return 1;
	}
	return 0;
}

/*
 * either direct or indirect
 */
int
copyau(Adr *a, Adr *v)
{

	if(copyas(a, v))
		return 1;
	if(v->type == D_REG) {
		if(a->type == D_OREG) {
			if(v->reg == a->reg)
				return 1;
		}
	}
	return 0;
}

/*
 * substitute s for v in a
 * return failure to substitute
 */
int
copysub(Adr *a, Adr *v, Adr *s, int f)
{

	if(f)
	if(copyau(a, v)) {
		a->reg = s->reg;
	}
	return 0;
}
