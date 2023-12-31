#include "gc.h"

static int
commutes(int o)
{
	switch(o){
		case OAND:
		case OOR:
		case OXOR:
		case OLMUL:
		case OMUL:
		case OADD:
			return 1;
	}
	return 0;
}

void
cgen(Node *n, Node *nn)
{
	Node *l, *r;
	Prog *p1;
	Node nod, nod1, nod2, nod3, nod4;
	int o, ol, t;
	long v, curs;

	if(debug['g'] && nn == Z) {
		// prtree(nn, "cgen lhs");
		prtree(n, "cgen");
	}
	if(n == Z || n->type == T)
		return;
	if(typesuv[n->type->etype]) {
		sugen(n, nn, n->type->width);
		return;
	}
	l = n->left;
	r = n->right;
	o = n->op;
	if(n->addable >= INDEXED) {
		if(nn == Z) {
			switch(o) {
			default:
				nullwarn(Z, Z);
				break;
			case OINDEX:
				nullwarn(l, r);
				break;
			}
			return;
		}
		gmove(n, nn);
		return;
	}
	curs = cursafe;

	if(n->complex >= FNX)
	if(l->complex >= FNX)
	if(r != Z && r->complex >= FNX)
	switch(o) {
	default:
		regret(&nod, r);
		cgen(r, &nod);

		regsalloc(&nod1, r);
		gopcode(OAS, &nod, &nod1);

		regfree(&nod);
		nod = *n;
		nod.right = &nod1;
		cgen(&nod, nn);
		return;

	case OFUNC:
	case OCOMMA:
	case OANDAND:
	case OOROR:
	case OCOND:
	case ODOT:
		break;
	}

	switch(o) {
	default:
		diag(n, "unknown op in cgen: %O", o);
		break;

	case OAS:
		if(l->op == OBIT)
			goto bitas;
		if(l->addable >= INDEXED && l->complex < FNX) {
			if(nn != Z || r->addable < INDEXED) {
				if(r->complex >= FNX && nn == Z)
					regret(&nod, r);
				else
					regalloc(&nod, r, nn, 0);
				cgen(r, &nod);
				gmove(&nod, l);
				if(nn != Z)
					gmove(&nod, nn);
				regfree(&nod);
			} else
				gmove(r, l);
			break;
		}
		if(l->complex >= r->complex) {
			reglcgen(&nod1, l, Z);
			if(r->addable >= INDEXED) {
				gmove(r, &nod1);
				if(nn != Z)
					gmove(r, nn);
				regfree(&nod1);
				break;
			}
			regalloc(&nod, r, nn, 0);
			cgen(r, &nod);
		} else {
			regalloc(&nod, r, nn, 0);
			cgen(r, &nod);
			reglcgen(&nod1, l, Z);
		}
		gmove(&nod, &nod1);
		regfree(&nod);
		regfree(&nod1);
		break;

	bitas:
		n = l->left;
		regalloc(&nod, r, nn, 0);
		if(l->complex >= r->complex) {
			reglcgen(&nod1, n, Z);
			cgen(r, &nod);
		} else {
			cgen(r, &nod);
			reglcgen(&nod1, n, Z);
		}
		regalloc(&nod2, n, Z, 0);
		gopcode(OAS, &nod1, &nod2);
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;

	case OBIT:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		bitload(n, &nod, Z, Z, nn);
		gopcode(OAS, &nod, nn);
		regfree(&nod);
		break;

	case ODIV:
	case OMOD:
		if(nn != Z)
		if(o == ODIV && (t = vlog(r)) >= 0 && t <= 8) {
			/* signed div/mod by constant power of 2 */
			cgen(l, nn);
			gopcode(OGE, nodconst(0, nn), nn);
			p1 = p;
			if(o == ODIV) {
				gopcode(OADD, nodconst((1<<t)-1, nn), nn);
				patch(p1, pc);
				gopcode(OASHR, nodconst(t, nn), nn);
			} else {
				gopcode(ONEG, Z, nn);
				regalloc(&nod, l, Z, 0);
				gopcode(OAS, nodconst((1<<t)-1, &nod), &nod);
				gopcode(OAND, &nod, nn);
				regfree(&nod);
				// gopcode(OAND, nodconst((1<<t)-1, nn), nn);
				gopcode(ONEG, Z, nn);
				gbranch(OGOTO);
				patch(p1, pc);
				p1 = p;
				regalloc(&nod, l, Z, 0);
				gopcode(OAS, nodconst((1<<t)-1, &nod), &nod);
				gopcode(OAND, &nod, nn);
				regfree(&nod);
				// gopcode(OAND, nodconst((1<<t)-1, nn), nn);
				patch(p1, pc);
			}
			break;
		}
		goto muldiv;

	case OADD:
	case OSUB:
	case OLSHR:
	case OASHL:
	case OASHR:
		if(r->op == OCONST && r->vconst == 0){
			cgen(l, nn);
			break;
		}
		/*
		 * immediate operands
		 */
		if(nn != Z)
		if(o != OSUB || hireg(nn))
		if(sconst(r))
		if(o != OADD) {
			cgen(l, nn);
			if(r->vconst == 0)
				break;
			gopcode(o, r, nn);
			break;
		}

	case OAND:
	case OOR:
	case OXOR:
	case OLMUL:
	case OLDIV:
	case OLMOD:
	case OMUL:
	muldiv:
		if(nn == Z) {
			nullwarn(l, r);
			break;
		}
		if(o == OMUL || o == OLMUL) {
			if(mulcon(n, nn, 0))
				break;
		}
		if(nn != Z)
		if(hireg(nn))
		if(sconst(r))
		if(o == OAND || o == OOR) {
			cgen(l, nn);
			if(o == OOR && r->vconst == 0)
				break;
			gopcode(o, r, nn);
			break;
		}
		if(l->complex >= r->complex) {
			regalloc(&nod, l, nn, o == OMUL);
			cgen(l, &nod);
			regalloc(&nod1, r, Z, o == OMUL);
			cgen(r, &nod1);
			gopcode(o, &nod1, &nod);
		} else {
			if(0 && o == OADD || 0 && o == OSUB) {
				regalloc(&nod, l, nn, 0);
				cgen(l, &nod);
				regalloc(&nod1, r, Z, 0);
				cgen(r, &nod1);
				gopcode(o, &nod1, &nod);
			}
			else {
				if(commutes(o)){
					regalloc(&nod, r, nn, o == OMUL);
					cgen(r, &nod);
					regalloc(&nod1, l, Z, o == OMUL);
					cgen(l, &nod1);
					gopcode(o, &nod1, &nod);
				}
				else{
					regalloc(&nod1, r, Z, 0);
					cgen(r, &nod1);
					regalloc(&nod, l, nn, 0);
					cgen(l, &nod);
					gopcode(o, &nod1, &nod);
				}
			}
		}
		gopcode(OAS, &nod, nn);
		regfree(&nod);
		regfree(&nod1);
		break;

	case ONEG:
	case OCOM:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		regalloc(&nod, l, nn, 0);
		cgen(l, &nod);
		gopcode(o, Z, &nod);
		gopcode(OAS, &nod, nn);
		regfree(&nod);
		break;
		
	case OASLSHR:
	case OASASHL:
	case OASASHR:
	case OASADD:
	case OASSUB:
		if(l->op == OBIT)
			goto asbitop;
		if(sconst(r)){	/* was just OASSUB */
			if(l->addable < INDEXED)
				reglcgen(&nod2, l, Z);
			else
				nod2 = *l;
			regalloc(&nod, r, nn, 0);
			gopcode(OAS, &nod2, &nod);
			gopcode(o, r, &nod);
			gopcode(OAS, &nod, &nod2);
	
			regfree(&nod);
			if(l->addable < INDEXED)
				regfree(&nod2);
			break;
		}

	case OASAND:
	case OASXOR:
	case OASOR:
	case OASLMUL:
	case OASLDIV:
	case OASLMOD:
	case OASMUL:
	case OASDIV:
	case OASMOD:
		if(l->op == OBIT)
			goto asbitop;
		if(l->complex >= r->complex) {
			if(l->addable < INDEXED)
				reglcgen(&nod2, l, Z);
			else
				nod2 = *l;
			regalloc(&nod1, r, Z, o == OASMUL);
			cgen(r, &nod1);
		} else {
			regalloc(&nod1, r, Z, o == OASMUL);
			cgen(r, &nod1);
			if(l->addable < INDEXED)
				reglcgen(&nod2, l, Z);
			else
				nod2 = *l;
		}

		regalloc(&nod, n, nn, o == OASMUL);
		gmove(&nod2, &nod);
		gopcode(o, &nod1, &nod);
		gmove(&nod, &nod2);
		if(nn != Z)
			gopcode(OAS, &nod, nn);
		regfree(&nod);
		regfree(&nod1);
		if(l->addable < INDEXED)
			regfree(&nod2);
		break;

	asbitop:
		regalloc(&nod4, n, nn, 0);
		if(l->complex >= r->complex) {
			bitload(l, &nod, &nod1, &nod2, &nod4);
			regalloc(&nod3, r, Z, 0);
			cgen(r, &nod3);
		} else {
			regalloc(&nod3, r, Z, 0);
			cgen(r, &nod3);
			bitload(l, &nod, &nod1, &nod2, &nod4);
		}
		gmove(&nod, &nod4);
		gopcode(o, &nod3, &nod4);
		regfree(&nod3);
		gmove(&nod4, &nod);
		regfree(&nod4);
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;

	case OADDR:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		lcgen(l, nn);
		break;

	case OFUNC:
		if(l->complex >= FNX) {
			if(l->op != OIND)
				diag(n, "bad function call");

			regret(&nod, l->left);
			cgen(l->left, &nod);
			regsalloc(&nod1, l->left);
			gopcode(OAS, &nod, &nod1);
			regfree(&nod);

			nod = *n;
			nod.left = &nod2;
			nod2 = *l;
			nod2.left = &nod1;
			nod2.complex = 1;
			cgen(&nod, nn);

			return;
		}
		if(REGARG >= 0){
			o = reg[REGARG];
			ol = reg[REGARGL];
		}
		gargs(r, &nod, &nod1);
		if(l->addable < INDEXED) {
			reglcgen(&nod, l, Z);
			gopcode(OFUNC, Z, &nod);
			regfree(&nod);
		} else
			gopcode(OFUNC, Z, l);
		if(REGARG >= 0)
			if(o != reg[REGARG] || ol != reg[REGARGL])
				regargfree();
		if(nn != Z) {
			regret(&nod, n);
			gopcode(OAS, &nod, nn);
			regfree(&nod);
		}
		break;

	case OIND:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		regialloc(&nod, n, nn);
		r = l;
		while(r->op == OADD)
			r = r->right;
		if(sconst(r) && (v = r->vconst+nod.xoffset) >= 0 && v < 64) {
			v = r->vconst;
			r->vconst = 0;
			cgen(l, &nod);
			nod.xoffset += v;
			r->vconst = v;
		} else
			cgen(l, &nod);
		regind(&nod, n);
		gopcode(OAS, &nod, nn);
		regfree(&nod);
		break;

	case OEQ:
	case ONE:
	case OLE:
	case OLT:
	case OGE:
	case OGT:
	case OLO:
	case OLS:
	case OHI:
	case OHS:
		if(nn == Z) {
			nullwarn(l, r);
			break;
		}
		boolgen(n, 1, nn);
		break;

	case OANDAND:
	case OOROR:
		boolgen(n, 1, nn);
		if(nn == Z)
			patch(p, pc);
		break;

	case ONOT:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		boolgen(n, 1, nn);
		break;

	case OCOMMA:
		cgen(l, Z);
		cgen(r, nn);
		break;

	case OCAST:
		if(nn == Z) {
			nullwarn(l, Z);
			break;
		}
		/*
		 * convert from types l->n->nn
		 */
		if(nocast(l->type, n->type)) {
			if(nocast(n->type, nn->type)) {
				cgen(l, nn);
				break;
			}
		}
		regalloc(&nod, l, nn, 0);
		cgen(l, &nod);
		regalloc(&nod1, n, &nod, 0);
		gopcode(OAS, &nod, &nod1);
		gopcode(OAS, &nod1, nn);
		regfree(&nod1);
		regfree(&nod);
		break;

	case ODOT:
		sugen(l, nodrat, l->type->width);
		if(nn != Z) {
			warn(n, "non-interruptable temporary");
			nod = *nodrat;
			if(!r || r->op != OCONST) {
				diag(n, "DOT and no offset");
				break;
			}
			nod.xoffset += (long)r->vconst;
			nod.type = n->type;
			cgen(&nod, nn);
		}
		break;

	case OCOND:
		bcgen(l, 1);
		p1 = p;
		cgen(r->left, nn);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		cgen(r->right, nn);
		patch(p1, pc);
		break;

	case OPOSTINC:
	case OPOSTDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(o == OPOSTDEC)
			v = -v;
		if(l->op == OBIT)
			goto bitinc;
		if(nn == Z)
			goto pre;

		if(l->addable < INDEXED)
			reglcgen(&nod2, l, Z);
		else
			nod2 = *l;

		regalloc(&nod, l, nn, 0);
		gopcode(OAS, &nod2, &nod);
		regalloc(&nod1, l, Z, 0);
		if(typefd[l->type->etype]) {
			regalloc(&nod3, l, Z, 0);
			if(v < 0) {
				gopcode(OAS, nodfconst(-v), &nod3);
				gopcode(OAS, &nod, &nod1);
				gopcode(OSUB, &nod3, &nod1);
			} else {
				gopcode(OAS, nodfconst(v), &nod3);
				gopcode(OAS, &nod, &nod1);
				gopcode(OADD, &nod3, &nod1);
			}
			regfree(&nod3);
		} else {
			if (v > 0 && v < 8){
				gopcode(OAS, nodconst(v, &nod1), &nod1);
				gopcode(OADD, &nod, &nod1);
			}
			else if (v < 0 && v > -8){
				gopcode(OAS, &nod, &nod1);
				gopcode(OSUB, nodconst(-v, &nod1), &nod1);
			}
			else {
				regalloc(&nod3, l, Z, 0);
				gopcode(OAS, nodconst(v, &nod3), &nod3);
				gopcode(OAS, &nod, &nod1);
				gopcode(OADD, &nod3, &nod1);
				regfree(&nod3);
			}
		}
		gopcode(OAS, &nod1, &nod2);

		regfree(&nod);
		regfree(&nod1);
		if(l->addable < INDEXED)
			regfree(&nod2);
		break;

	case OPREINC:
	case OPREDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(o == OPREDEC)
			v = -v;
		if(l->op == OBIT)
			goto bitinc;

	pre:
		if(l->addable < INDEXED)
			reglcgen(&nod2, l, Z);
		else
			nod2 = *l;

		regalloc(&nod, l, nn, 0);
		gopcode(OAS, &nod2, &nod);
		if(typefd[l->type->etype]) {
			regalloc(&nod3, l, Z, 0);
			if(v < 0) {
				gopcode(OAS, nodfconst(-v), &nod3);
				gopcode(OSUB, &nod3, &nod);
			} else {
				gopcode(OAS, nodfconst(v), &nod3);
				gopcode(OADD, &nod3, &nod);
			}
			regfree(&nod3);
		} else {
			if (v > 0 && v < 256)
				gopcode(OADD, nodconst(v, &nod), &nod);
			else if (v < 0 && v > -256)
				gopcode(OSUB, nodconst(-v, &nod), &nod);
			else {
				regalloc(&nod3, l, Z, 0);
				gopcode(OAS, nodconst(v, &nod3), &nod3);
				gopcode(OADD, &nod3, &nod);
				regfree(&nod3);
			}
		}
		gopcode(OAS, &nod, &nod2);

		regfree(&nod);
		if(l->addable < INDEXED)
			regfree(&nod2);
		break;

	bitinc:
		if(nn != Z && (o == OPOSTINC || o == OPOSTDEC)) {
			bitload(l, &nod, &nod1, &nod2, Z);
			gopcode(OAS, &nod, nn);
			if (v > 0 && v < 256)
				gopcode(OADD, nodconst(v, &nod), &nod);
			else if (v < 0 && v > -256)
				gopcode(OSUB, nodconst(-v, &nod), &nod);
			else {
				regalloc(&nod3, l, Z, 0);
				gopcode(OAS, nodconst(v, &nod3), &nod3);
				gopcode(OADD, &nod3, &nod);
				regfree(&nod3);
			}
			bitstore(l, &nod, &nod1, &nod2, Z);
			break;
		}
		bitload(l, &nod, &nod1, &nod2, nn);
		if (v > 0 && v < 256)
			gopcode(OADD, nodconst(v, &nod), &nod);
		else if (v < 0 && v > -256)
			gopcode(OSUB, nodconst(-v, &nod), &nod);
		else {
			regalloc(&nod3, l, Z, 0);
			gopcode(OAS, nodconst(v, &nod3), &nod3);
			gopcode(OADD, &nod3, &nod);
			regfree(&nod3);
		}
		bitstore(l, &nod, &nod1, &nod2, nn);
		break;
	}
	cursafe = curs;
	return;
}

void
reglcgen(Node *t, Node *n, Node *nn)
{
	Node *r;
	long v;

	regialloc(t, n, nn);
	if(n->op == OIND) {
		r = n->left;
		while(r->op == OADD)
			r = r->right;
		if(sconst(r) && (v = r->vconst+t->xoffset) >= 0 && v < 64) {
			v = r->vconst;
			r->vconst = 0;
			lcgen(n, t);
			t->xoffset += v;
			r->vconst = v;
			regind(t, n);
			return;
		}
	} else if(n->op == OINDREG) {
		if((v = n->xoffset) >= 0 && v < 64) {
			n->op = OREGISTER;
			cgen(n, t);
			t->xoffset += v;
			n->op = OINDREG;
			regind(t, n);
			return;
		}
	}
	lcgen(n, t);
	regind(t, n);
}

void
reglpcgen(Node *n, Node *nn, int f)
{
	Type *t;

	t = nn->type;
	nn->type = types[TLONG];
	if(f)
		reglcgen(n, nn, Z);
	else {
		regialloc(n, nn, Z);
		lcgen(nn, n);
		regind(n, nn);
	}
	nn->type = t;
}

void
lcgen(Node *n, Node *nn)
{
	Prog *p1;
	Node nod;

	if(debug['g']) {
		prtree(nn, "lcgen lhs");
		prtree(n, "lcgen");
	}
	if(n == Z || n->type == T)
		return;
	if(nn == Z) {
		nn = &nod;
		regalloc(&nod, n, Z, 0);
	}
	switch(n->op) {
	default:
		if(n->addable < INDEXED) {
			diag(n, "unknown op in lcgen: %O", n->op);
			break;
		}
		nod = *n;
		nod.op = OADDR;
		nod.left = n;
		nod.right = Z;
		nod.type = types[TIND];
		gopcode(OAS, &nod, nn);
		break;

	case OCOMMA:
		cgen(n->left, n->left);
		lcgen(n->right, nn);
		break;

	case OIND:
		cgen(n->left, nn);
		break;

	case OCOND:
		bcgen(n->left, 1);
		p1 = p;
		lcgen(n->right->left, nn);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		lcgen(n->right->right, nn);
		patch(p1, pc);
		break;
	}
}

void
bcgen(Node *n, int true)
{

	if(n->type == T)
		gbranch(OGOTO);
	else
		boolgen(n, true, Z);
}

void
boolgen(Node *n, int true, Node *nn)
{
	int o;
	Prog *p1, *p2;
	Node *l, *r, nod, nod1;
	long curs;

	if(debug['g']) {
		prtree(nn, "boolgen lhs");
		prtree(n, "boolgen");
	}
	curs = cursafe;
	l = n->left;
	r = n->right;
	switch(n->op) {

	default:
		regalloc(&nod, n, nn, 0);
		cgen(n, &nod);
		o = ONE;
		if(true)
			o = comrel[relindex(o)];
		if(typefd[n->type->etype]) {
			gopcode(o, nodfconst(0), &nod);
		} else
			gopcode(o, nodconst(0, &nod), &nod);
		regfree(&nod);
		goto com;

	case OCONST:
		o = vconst(n);
		if(!true)
			o = !o;
		gbranch(OGOTO);
		if(o) {
			p1 = p;
			gbranch(OGOTO);
			patch(p1, pc);
		}
		goto com;

	case OCOMMA:
		cgen(l, Z);
		boolgen(r, true, nn);
		break;

	case ONOT:
		boolgen(l, !true, nn);
		break;

	case OCOND:
		bcgen(l, 1);
		p1 = p;
		bcgen(r->left, true);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		bcgen(r->right, !true);
		patch(p2, pc);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		goto com;

	case OANDAND:
		if(!true)
			goto caseor;

	caseand:
		bcgen(l, true);
		p1 = p;
		bcgen(r, !true);
		p2 = p;
		patch(p1, pc);
		gbranch(OGOTO);
		patch(p2, pc);
		goto com;

	case OOROR:
		if(!true)
			goto caseand;

	caseor:
		bcgen(l, !true);
		p1 = p;
		bcgen(r, !true);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		goto com;

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
		o = n->op;
		if(true)
			o = comrel[relindex(o)];
		if(l->complex >= FNX && r->complex >= FNX) {
			regret(&nod, r);
			cgen(r, &nod);
			regsalloc(&nod1, r);
			gopcode(OAS, &nod, &nod1);
			regfree(&nod);
			nod = *n;
			nod.right = &nod1;
			boolgen(&nod, true, nn);
			break;
		}
		if(sconst(l)) {
			regalloc(&nod, r, nn, 0);
			cgen(r, &nod);
			o = invrel[relindex(o)];
			gopcode(o, l, &nod);
			regfree(&nod);
			goto com;
		}
		if(sconst(r)) {
			regalloc(&nod, l, nn, 0);
			cgen(l, &nod);
			gopcode(o, r, &nod);
			regfree(&nod);
			goto com;
		}
		if(l->complex >= r->complex) {
			regalloc(&nod1, l, nn, 0);
			cgen(l, &nod1);
			regalloc(&nod, r, Z, 0);
			cgen(r, &nod);
		} else {
			regalloc(&nod, r, nn, 0);
			cgen(r, &nod);
			regalloc(&nod1, l, Z, 0);
			cgen(l, &nod1);
		}
		gopcode(o, &nod, &nod1);
		regfree(&nod);
		regfree(&nod1);

	com:
		if(nn != Z) {
			p1 = p;
			gopcode(OAS, nodconst(1, nn), nn);
			gbranch(OGOTO);
			p2 = p;
			patch(p1, pc);
			gopcode(OAS, nodconst(0, nn), nn);
			patch(p2, pc);
		}
		break;
	}
	cursafe = curs;
}

void
sugen(Node *n, Node *nn, long w)
{
	Prog *p1;
	Node nod0, nod1, nod2, nod3, nod4, *l, *r;
	Type *t, *ct, *rt;
	long pc1;
	int i, m, c;

	if(n == Z || n->type == T)
		return;
	if(debug['g']) {
		prtree(nn, "sugen lhs");
		prtree(n, "sugen");
	}
	if(nn == nodrat)
		if(w > nrathole)
			nrathole = w;
	switch(n->op) {
	case OIND:
		if(nn == Z) {
			nullwarn(n->left, Z);
			break;
		}

	default:
		goto copy;

	case OCONST:
		if(n->type && typev[n->type->etype]) {
			if(nn == Z) {
				nullwarn(n->left, Z);
				break;
			}

			t = nn->type;
			nn->type = types[TLONG];
			reglcgen(&nod1, nn, Z);
			nn->type = t;

			gopcode(OAS, nod32const(n->vconst>>32, &nod1), &nod1);
			nod1.xoffset += SZ_LONG;
			gopcode(OAS, nod32const(n->vconst, &nod1), &nod1);

			regfree(&nod1);
			break;
		}
		goto copy;

	case ODOT:
		l = n->left;
		sugen(l, nodrat, l->type->width);
		if(nn != Z) {
			warn(n, "non-interruptable temporary");
			nod1 = *nodrat;
			r = n->right;
			if(!r || r->op != OCONST) {
				diag(n, "DOT and no offset");
				break;
			}
			nod1.xoffset += (long)r->vconst;
			nod1.type = n->type;
			sugen(&nod1, nn, w);
		}
		break;

	case OSTRUCT:
		/*
		 * rewrite so lhs has no fn call
		 */
		if(nn != Z && nn->complex >= FNX) {
			nod1 = *n;
			nod1.type = typ(TIND, n->type);
			regret(&nod2, &nod1);
			lcgen(nn, &nod2);
			regsalloc(&nod0, &nod1);
			gopcode(OAS, &nod2, &nod0);
			regfree(&nod2);

			nod1 = *n;
			nod1.op = OIND;
			nod1.left = &nod0;
			nod1.right = Z;
			nod1.complex = 1;

			sugen(n, &nod1, w);
			return;
		}

		r = n->left;
		for(t = n->type->link; t != T; t = t->down) {
			l = r;
			if(r->op == OLIST) {
				l = r->left;
				r = r->right;
			}
			if(nn == Z) {
				cgen(l, nn);
				continue;
			}
			/*
			 * hand craft *(&nn + o) = l
			 */
			nod0 = znode;
			nod0.op = OAS;
			nod0.type = t;
			nod0.left = &nod1;
			nod0.right = l;

			nod1 = znode;
			nod1.op = OIND;
			nod1.type = t;
			nod1.left = &nod2;

			nod2 = znode;
			nod2.op = OADD;
			nod2.type = typ(TIND, t);
			nod2.left = &nod3;
			nod2.right = &nod4;

			nod3 = znode;
			nod3.op = OADDR;
			nod3.type = nod2.type;
			nod3.left = nn;

			nod4 = znode;
			nod4.op = OCONST;
			nod4.type = nod2.type;
			nod4.vconst = t->offset;

			ccom(&nod0);
			acom(&nod0);
			xcom(&nod0);
			nod0.addable = 0;

			cgen(&nod0, Z);
		}
		break;

	case OAS:
		if(nn == Z) {
			if(n->addable < INDEXED)
				sugen(n->right, n->left, w);
			break;
		}
		sugen(n->right, nodrat, w);
		warn(n, "non-interruptable temporary");
		sugen(nodrat, n->left, w);
		sugen(nodrat, nn, w);
		break;

	case OFUNC:
		if(nn == Z) {
			sugen(n, nodrat, w);
			break;
		}
		if(nn->op != OIND) {
			nn = new1(OADDR, nn, Z);
			nn->type = types[TIND];
			nn->addable = 0;
		} else
			nn = nn->left;
		n = new(OFUNC, n->left, new(OLIST, nn, n->right));
		n->type = types[TVOID];
		n->left->type = types[TVOID];
		cgen(n, Z);
		break;

	case OCOND:
		bcgen(n->left, 1);
		p1 = p;
		sugen(n->right->left, nn, w);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;
		sugen(n->right->right, nn, w);
		patch(p1, pc);
		break;

	case OCOMMA:
		cgen(n->left, Z);
		sugen(n->right, nn, w);
		break;
	}
	return;

copy:
	if(nn == Z)
		return;
	if(n->complex >= FNX && nn->complex >= FNX) {
		t = nn->type;
		nn->type = types[TLONG];
		regialloc(&nod1, nn, Z);
		lcgen(nn, &nod1);
		regsalloc(&nod2, nn);
		nn->type = t;

		gopcode(OAS, &nod1, &nod2);
		regfree(&nod1);

		nod2.type = typ(TIND, t);

		nod1 = nod2;
		nod1.op = OIND;
		nod1.left = &nod2;
		nod1.right = Z;
		nod1.complex = 1;
		nod1.type = t;

		sugen(n, &nod1, w);
		return;
	}

	if(n->complex > nn->complex) {
		t = n->type;
		n->type = types[TLONG];
		reglcgen(&nod1, n, Z);
		n->type = t;

		t = nn->type;
		nn->type = types[TLONG];
		reglcgen(&nod2, nn, Z);
		nn->type = t;
	} else {
		t = nn->type;
		nn->type = types[TLONG];
		reglcgen(&nod2, nn, Z);
		nn->type = t;

		t = n->type;
		n->type = types[TLONG];
		reglcgen(&nod1, n, Z);
		n->type = t;
	}

	if(w%SZ_LONG == SZ_INT){
		nod1.type = nod2.type = regnode.type = types[TINT];
		regalloc(&nod3, &regnode, Z, 0);
		gopcode(OAS, &nod1, &nod3);
		gopcode(OAS, &nod3, &nod2);
		nod1.xoffset += SZ_INT;
		nod2.xoffset += SZ_INT;
		regfree(&nod3);
		nod1.type = nod2.type = regnode.type = types[TLONG];
	}

	w /= SZ_LONG;
	if(w <= 1) {
		layout(&nod1, &nod2, w, 0, Z);
		goto out;
	}

	/*
	 * minimize space for unrolling loop
	 * 3,4,5 times. (6 or more is never minimum)
	 * if small structure, try 2 also.
	 */
	c = 0; /* set */
	m = 100;
	i = 3;
	if(w <= 15)
		i = 2;
	for(; i<=5; i++)
		if(i + w%i <= m) {
			c = i;
			m = c + w%c;
		}

	c = 1;

	if(w/c < 256)
		ct = types[TUCHAR];
	else if(w/c < 65536)
		ct = types[TUSHORT];
	else
		ct = types[TULONG];
	rt = regnode.type;
	regnode.type = ct;
	regalloc(&nod3, &regnode, Z, 0);
	regnode.type = rt;
	layout(&nod1, &nod2, w%c, w/c, &nod3);
	
	pc1 = pc;
	layout(&nod1, &nod2, c, 0, Z);

	gopcode(OSUB, nodconst(1L, &nod3), &nod3);
	nod1.op = OREGISTER;
	nod1.type = types[TINT];
	gopcode(OADD, nodconst(c*SZ_LONG, &nod1), &nod1);
	nod2.op = OREGISTER;
	nod2.type = types[TINT];
	gopcode(OADD, nodconst(c*SZ_LONG, &nod2), &nod2);
	
	gopcode(OGT, nodconst(0, &nod3), &nod3);
	patch(p, pc1);

	regfree(&nod3);

	nod1.op = nod2.op = OINDREG;
	nod1.type = nod2.type = types[TLONG];
out:
	regfree(&nod1);
	regfree(&nod2);
}

void
layout(Node *f, Node *t, int c, int cv, Node *cn)
{
	Node t1, t2;

	while(c > 3) {
		layout(f, t, 2, 0, Z);
		c -= 2;
	}

	regalloc(&t1, &regnode, Z, 0);
	regalloc(&t2, &regnode, Z, 0);
	if(c > 0) {
		gopcode(OAS, f, &t1);
		f->xoffset += SZ_LONG;
	}
	if(cn != Z)
		gopcode(OAS, nodconst(cv, cn), cn);
	if(c > 1) {
		gopcode(OAS, f, &t2);
		f->xoffset += SZ_LONG;
	}
	if(c > 0) {
		gopcode(OAS, &t1, t);
		t->xoffset += SZ_LONG;
	}
	if(c > 2) {
		gopcode(OAS, f, &t1);
		f->xoffset += SZ_LONG;
	}
	if(c > 1) {
		gopcode(OAS, &t2, t);
		t->xoffset += SZ_LONG;
	}
	if(c > 2) {
		gopcode(OAS, &t1, t);
		t->xoffset += SZ_LONG;
	}
	regfree(&t1);
	regfree(&t2);
}
