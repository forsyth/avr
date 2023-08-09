#include "l.h"

long	op(int), opi(int, int), opd(int, int), opid(int, int, int), oprd(int, int, int), opbr(int, int), oplbr(int, int, long*), opbrz(int), opbrbs(int, int, int), opbrb(int, int), ldstxyz(int, int, int, int), ldstdata(int, int);

static int regmask(void);

static void
loreg(Prog *p, int c, int r)
{
	if(r >= 16)
		diag("high reg %d when low expected in case %d: %P", r, c, p);
}

static void
hireg(Prog *p, int c, int r)
{
	if(r < 16)
		diag("low reg %d when high expected in case %d: %P", r, c, p);
}

static void
ppreg(Prog *p, int c, int r)
{
	if(r != REGW && r != REGX && r != REGY && r != REGZ)
		diag("low reg %d when 24, 26, 28 or 30 expected in case %d: %P", r, c, p);
}

static void
xyzreg(Prog *p, int c, int r)
{
	if(r != REGX && r != REGY && r != REGZ)
		diag("low reg %d when 26 or 28 or 30 expected in case %d: %P", r, c, p);
}

static void
yzreg(Prog *p, int c, int r)
{
	if(r != REGY && r != REGZ)
		diag("low reg %d when 28 or 30 expected in case %d: %P", r, c, p);
}

static void
notz(Prog *p, int c, int r)
{
	if(r == REGZ)
		diag("Z reg in case %d: %P", c, p);
}

static void
even(Prog *p, int c, int r)
{
	if(r&1)
		diag("odd reg %d when even expected in case %d: %P", r, c, p);
}

static int regs;

int
asmout(Prog *p, Optab *o, int gen)
{
	long o1, o2, o3, o4, o5, o6, o7, o8, o9, v, b;
	int a, rs, rd;

	regs = 0;
	a = p->as;
	rs = p->from.reg;
	rd = p->to.reg;
	v = 0;
	if(p->from.type == D_CONST || p->from.type == D_OREG){
		aclass(&p->from, p);
		v = instoffset;
	}
	if(p->to.type == D_CONST || p->to.type == D_OREG){
		aclass(&p->to, p);
		v = instoffset;
	}
	o1 = 0;
	o2 = 0;
	o3 = 0;
	o4 = 0;
	o5 = 0;
	o6 = 0;
	o7 = 0;
	o8 = 0;
	o9 = 0;
	switch(o->type) {
	default:
		diag("unknown type %d", o->type);
		if(!debug['a'])
			prasm(p);
		break;
	case 0:		/* pseudo ops */
		break;
	case 1:		/* reg, reg byte/word */
		if(a == AMULB || a == AMULSUB){
			hireg(p, 1, rs);
			hireg(p, 1, rd);
		}
		if(a == AMOVW){
			even(p, 1, rs);
			even(p, 1, rd);
		}
		o1 = oprd(a, rs, rd);
		break;
	case 2:		/* imm, reg byte */
		if(a == AADDW || a == ASUBW)
			ppreg(p, 2, rd);
		if(a == ASUBB || a == ASUBBC || a == AANDB || a == AORB || a == AMOVB || a == ACMPB || a == ACBRB || a == ASBRB)
			hireg(p, 2, rd);
		o1 = opid(a, v, rd);
		break;
	case 3:		/* reg byte */
		o1 = opd(a, rd);
		break;
	case 4:		/* addw, addwc */
		o1 = oprd(a == AADDW ? AADDB : AADDBC, rs, rd);
		o2 = oprd(AADDBC, rs+1, rd+1);
		break;
	case 5:		/* long movw */
		o1 = oprd(AMOVB, rs, rd);
		o2 = oprd(AMOVB, rs+1, rd+1);
		break;
	case 6:		/* break etc */
		o1 = op(a);
		break;
	case 7:		/* bclr etc */
		o1 = opi(a, v);
		break;
	case 8:		/* short abr or call */
		b = (p->pcond->pc - pc)/2 - 1;
		if(gen && (b < -2048 || b >= 2047))
			diag("short branch too long: %P", p);
		o1 = opbr(a, b);
		break;
	case 9:		/* long abr or call */
		b = p->pcond->pc/2;
		if(gen && (b < 0 || b >= (1<<22)))
			diag("long branch too long: %P", p);
		o1 = oplbr(a, b, &o2);
		break;
	case 10:		/* abr or call (r) */
		even(p, 10, rd);
		notz(p, 10, rd);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = opbrz(a);
		break;
	case 11:		/* abr or call o(r) */
		even(p, 11, rd);
		notz(p, 11, rd);
		if(v < 0 || v >= 64)
			diag("offset too big in abr o(r): %P", p);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = opid(AADDW, v, REGZ);
		o3 = opbrz(a);
		break;
	case 12:		/* brbs etc */
		b = (p->pcond->pc - pc)/2 - 1;
		if(gen && (b < -64 || b > 63))
			diag("short brbs too long: %P", p);
		o1 = opbrbs(a, b, v);
		break;
	case 13:		/* long brbs */
		b = p->pcond->pc/2;
		if(gen && (b < 0 || b >= (1<<22)))
			diag("long brbs too long: %P", p);
		o1 = opbrbs(relinv(a), 2, v);
		o2 = oplbr(ABR, b, &o3);
		break;
	case 14:		/* beq etc */
		b = (p->pcond->pc - pc)/2 - 1;
		if(gen && (b < -64 || b > 63))
			diag("short conditional branch too long: %P", p);
		o1 = opbrb(a, b);
		break;
	case 15:		/* long beq etc */
		b = p->pcond->pc/2;
		if(gen && (b < 0 || b >= (1<<22)))
			diag("long cbranch too long: %P", p);
		o1 = opbrb(relinv(a), 2);
		o2 = oplbr(ABR, b, &o3);
		break;
	case 16:		/* ret */
		o1 = a == ARET ? 0x9508 : 0x9518;
		break;
	case 17:		/* word */
		o1 = v;
		break;
	case 18:		/* movpm (Z), r or (Z)+, r */
		if(p->from.type == D_OREG)
			o1 = ldstxyz(0x9004, 0, rd, REGZ);
		else
			o1 = ldstxyz(0x9005, 0, rd, REGZ);
		break;
	case 19:		/* load o(r) */
		if(v < 0 || v >= 64)
			diag("long offset in load o(r): %P", p);
		if(rs == NREG)
			rs = REGSP;
		even(p, 19, rs);
		yzreg(p, 19, rs);
		if(rs == REGZ)
			o1 = ldstxyz(0x8000, v, rd, rs);
		else
			o1 = ldstxyz(0x8008, v, rd, rs);
		break;
	case 20:		/* store o(r) */
		if(v < 0 || v >= 64)
			diag("long offset in store o(r): %P", p);
		if(rd == NREG)
			rd = REGSP;
		even(p, 20, rd);
		yzreg(p, 20, rd);
		if(rd == REGZ)
			o1 = ldstxyz(0x8200, v, rs, rd);
		else
			o1 = ldstxyz(0x8208, v, rs, rd);
		break;
	case 21:		/* load data */
		if(v < 0 || v > 65535)
			diag("long offset in load data: %P", p);
		o1 = ldstdata(0x9000, rd);
		o2 = v;
		break;
	case 22:		/* store data */
		if(v < 0 || v > 65535)
			diag("long offset in store data: %P", p);
		o1 = ldstdata(0x9200, rs);
		o2 = v;
		break;
	case 23:		/* load big o(r) */
		if(v >= 0 && v < 64)
			diag("short offset in load big o(r): %P", p);
		v = -v;
		if(rs == NREG)
			rs = REGSP;
		even(p, 23, rs);
		notz(p, 23, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x8000, 0, rd, REGZ);
		break;
	case 24:		/* store big o(r) */
		if(v >= 0 && v < 64)
			diag("short offset in store big o(r): %P", p);
		v = -v;
		if(rd == NREG)
			rd = REGSP;
		even(p, 24, rd);
		notz(p, 24, rs);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x8200, 0, rs, REGZ);
		break;
	case 25:		/* load data */
		diag("cannot do LEXT %P", p);
		break;
	case 26:		/* store data */
		diag("cannot do LEXT %P", p);
		break;
	case 27:		/* reg, reg word */
		a += AANDB - AANDW;
		o1 = oprd(a, rs, rd);
		o2 = oprd(a, rs+1, rd+1);
		break;
	case 28:		/* reg, reg long */
		a += AANDB - AANDL;
		o1 = oprd(a, rs, rd);
		o2 = oprd(a, rs+1, rd+1);
		o3 = oprd(a, rs+2, rd+2);
		o4 = oprd(a, rs+3, rd+3);
		break;
	case 29:		/* reg word */
		a += ACOMB - ACOMW;
		o1 = opd(a, rd);
		o2 = opd(a, rd+1);
		break;
	case 30:		/* reg long */
		a += ACOMB - ACOML;
		o1 = opd(a, rd);
		o2 = opd(a, rd+1);
		o3 = opd(a, rd+2);
		o4 = opd(a, rd+3);
		break;
	case 31:		/* addl */
		o1 = oprd(AADDB, rs, rd);
		o2 = oprd(AADDBC, rs+1, rd+1);
		o3 = oprd(AADDBC, rs+2, rd+2);
		o4 = oprd(AADDBC, rs+3, rd+3);
		break;
	case 32:		/* subw */
		o1 = oprd(a == ASUBW ? ASUBB : ASUBBC, rs, rd);
		o2 = oprd(ASUBBC, rs+1, rd+1);
		break;
	case 33:		/* subl */
		o1 = oprd(ASUBB, rs, rd);
		o2 = oprd(ASUBBC, rs+1, rd+1);
		o3 = oprd(ASUBBC, rs+2, rd+2);
		o4 = oprd(ASUBBC, rs+3, rd+3);
		break;
	case 34:		/* addw/subw c, hreg */
		hireg(p, 34, rd);
		if(a == AADDW)
			v = -v;
		o1 = opid(ASUBB, (v>>0)&0xff, rd);
		o2 = opid(ASUBBC, (v>>8)&0xff, rd+1);
		break;
	case 35:		/* subl imm */
		hireg(p, 35, rd);
		o1 = opid(ASUBB, (v>>0)&0xff, rd);
		o2 = opid(ASUBBC, (v>>8)&0xff, rd+1);
		o3 = opid(ASUBBC, (v>>16)&0xff, rd+2);
		o4 = opid(ASUBBC, (v>>24)&0xff, rd+3);
		break;
	case 36:		/* movw imm */
		hireg(p, 36, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, rd);
		o2 = opid(AMOVB, (v>>8)&0xff, rd+1);
		break;
	case 37:		/* movl r, r */
		even(p, 37, rs);
		even(p, 37, rd);
		o1 = oprd(AMOVW, rs, rd);
		o2 = oprd(AMOVW, rs+2, rd+2);
/*
		o1 = oprd(AMOVB, rs, rd);
		o2 = oprd(AMOVB, rs+1, rd+1);
		o3 = oprd(AMOVB, rs+2, rd+2);
		o4 = oprd(AMOVB, rs+3, rd+3);
*/
		break;
	case 38:		/* movl imm */
		hireg(p, 38, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, rd);
		o2 = opid(AMOVB, (v>>8)&0xff, rd+1);
		o3 = opid(AMOVB, (v>>16)&0xff, rd+2);
		o4 = opid(AMOVB, (v>>24)&0xff, rd+3);
		break;
	case 39:		/* cmpw r, r */
		diag("got to case 39");
		break;
	case 40:		/* cmpw imm, r */
		diag("got to case 40");
		break;
	case 41:		/* addw imm, addwc imm */
		loreg(p, 41, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGTMP);
		o2 = oprd(a == AADDW ? AADDB : AADDBC, REGTMP, rd);
		o3 = opid(AMOVB, (v>>8)&0xff, REGTMP);
		o4 = oprd(AADDBC, REGTMP, rd+1);
		break;
	case 42:		/* negw */
		hireg(p, 42, rd+1);
		o1 = opd(ACOMB, rd+1);
		o2 = opd(ANEGB, rd);
		o3 = opbrb(ABCS, 1);
		o4 = opid(ASUBB, -1, rd+1);
/*
		o1 = opd(ACOMB, rd);
		o2 = opd(ACOMB, rd+1);
		o3 = opid(ASUBB, 0xff, rd);
		o4 = opid(ASUBBC, 0xff, rd+1);
*/
		break;
	case 43:		/* negl */
		hireg(p, 43, rd+1);
		o1 = opd(ACOMB, rd+3);
		o2 = opd(ACOMB, rd+2);
		o3 = opd(ACOMB, rd+1);
		o4 = opd(ANEGB, rd);
		o5 = opbrb(ABCS, 3);
		o6 = opid(ASUBB, -1, rd+1);
		o7 = oprd(AADDBC, REGZERO, rd+2);
		o8 = oprd(AADDBC, REGZERO, rd+3);
		break;
	case 44:		/* alslw etc */
		o1 = opd(a-ALSLW+ALSLB, rd);
		o2 = opd(AROLB, rd+1);
		break;
	case 45:		/* alsrw etc */
		o1 = opd(a-ALSRW+ALSRB, rd+1);
		o2 = opd(ARORB, rd);
		break;
	case 46:		/* alsll etc */
		o1 = opd(a-ALSLL+ALSLB, rd);
		o2 = opd(AROLB, rd+1);
		o3 = opd(AROLB, rd+2);
		o4 = opd(AROLB, rd+3);
		break;
	case 47:		/* alsrl etc */
		o1 = opd(a-ALSRL+ALSRB, rd+3);
		o2 = opd(ARORB, rd+2);
		o3 = opd(ARORB, rd+1);
		o4 = opd(ARORB, rd);
		break;
	case 48:		/* movw $o(r), r */
		if(v < 0 || v >= 64)
			diag("long offset in movw $o(r): %P", p);
		if(rs == NREG)
			rs = REGSP;
		even(p, 48, rs);
		notz(p, 48, rd);
		hireg(p, 48, rd);
		even(p, 48, rd);
		ppreg(p, 48, rd);
		o1 = oprd(AMOVW, rs, rd);
		o2 = opid(AADDW, v, rd);
		break;
	case 49:		/* movw $o(r), r big */
		if(v >= 0 && v < 64)
			diag("short offset in movw $o(r): %P", p);
		v = -v;
		if(rs == NREG)
			rs = REGSP;
		even(p, 49, rs);
		notz(p, 49, rd);
		hireg(p, 49, rd);
		even(p, 49, rd);
		o1 = oprd(AMOVW, rs, rd);
		o2 = opid(ASUBB, v&0xff, rd);
		o3 = opid(ASUBBC, (v>>8)&0xff, rd+1);
		break;
	case 50:		/* abgt etc */
		b = (p->pcond->pc - pc)/2 - 1;
		if(gen && (b < -63 || b > 63))
			diag("abgt short conditional branch too long: %P", p);
		if(a == ABGT || a == ABHI)
			o1 = opbrb(ABEQ, 1);
		else
			o1 = opbrb(ABEQ, b);
		o2 = opbrb(relop(a), b-1);
		break;
	case 51:		/* long abgt etc */
		b = p->pcond->pc/2;
		if(gen && (b < 0 || b >= (1<<22)))
			diag("long abgt branch too long: %P", p);
		if(a == ABGT || a == ABHI)
			o1 = opbrb(ABEQ, 3);
		else
			o1 = opbrb(ABEQ, 1);
		o2 = opbrb(relop(relinv(a)), 2);
		o3 = oplbr(ABR, b, &o4);
		break;
	case 52:		/* case 2 but with low register */
		loreg(p, 52, rd);
		if(a == ACMPB || a == ACMPBC){
			o1 = oprd(AMOVB, rd, REGTMP);
			o2 = opid(a, v, REGTMP);
			break;
		}
		o1 = opid(AMOVB, v, REGTMP);
		o2 = oprd(a, REGTMP, rd);
		break;
	case 53:		/* subw c, reg */
		loreg(p, 53, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGTMP);
		o2 = oprd(a == ASUBW ? ASUBB : ASUBBC, REGTMP, rd);
		o3 = opid(AMOVB, (v>>8)&0xff, REGTMP);
		o4 = oprd(ASUBBC, REGTMP, rd+1);
		break;
	case 54:		/* mulb hreg, reg */
		loreg(p, 54, rd);
		o1 = oprd(AMOVB, rd, REGTMP);
		o2 = oprd(a, rs, REGTMP);
		break;
	case 55:		/* mulb reg, hreg */
		loreg(p, 55, rs);
		o1 = oprd(AMOVB, rs, REGTMP);
		o2 = oprd(a, REGTMP, rd);
		break;
	case 56:		/* mulb reg, reg */
		loreg(p, 56, rs);
		loreg(p, 56, rd);
		o1 = oprd(AMOVB, rs, REGTMP);
		o2 = oprd(AMOVB, rd, REGTMP+1);
		o3 = oprd(a, REGTMP, REGTMP+1);
		break;
	case 57:		/* pop word */
		a += APOPB - APOPW;
		o1 = opd(a, rd+1);
		o2 = opd(a, rd);
		break;
	case 58:		/* pop long */
		a += APOPB - APOPL;
		o1 = opd(a, rd+3);
		o2 = opd(a, rd+2);
		o3 = opd(a, rd+1);
		o4 = opd(a, rd);
		break;
	case 59:		/* movw $c, r - low */
		loreg(p, 59, rd);
		even(p, 59, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGTMP);
		o2 = opid(AMOVB, (v>>8)&0xff, REGTMP+1);
		o3 = oprd(AMOVW, REGTMP, rd);
		break;
	case 60:		/* movl $c, r - low */
		loreg(p, 60, rd);
		even(p, 60, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGTMP);
		o2 = opid(AMOVB, (v>>8)&0xff, REGTMP+1);
		o3 = oprd(AMOVW, REGTMP, rd);
		o4 = opid(AMOVB, (v>>16)&0xff, REGTMP);
		o5 = opid(AMOVB, (v>>24)&0xff, REGTMP+1);
		o6 = oprd(AMOVW, REGTMP, rd+2);
		break;
	case 61:		/* negw */
		loreg(p, 61, rd);
		o1 = opd(ACOMB, rd+1);
		o2 = opd(ANEGB, rd);
		o3 = opbrb(ABCS, 2);
		o4 = opid(AMOVB, 1, REGTMP);
		o5 = oprd(AADDB, REGTMP, rd+1);
		break;
	case 62:		/* negl */
		loreg(p, 62, rd);
		o1 = opd(ACOMB, rd+3);
		o2 = opd(ACOMB, rd+2);
		o3 = opd(ACOMB, rd+1);
		o4 = opd(ANEGB, rd);
		o5 = opbrb(ABCS, 4);
		o6 = opid(AMOVB, 1, REGTMP);
		o7 = oprd(AADDB, REGTMP, rd+1);
		o8 = oprd(AADDBC, REGZERO, rd+2);
		o9 = oprd(AADDBC, REGZERO, rd+3);
		break;
	case 63:		/* subl imm */
		loreg(p, 63, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGTMP);
		o2 = oprd(ASUBB, REGTMP, rd);
		o3 = opid(AMOVB, (v>>8)&0xff, REGTMP);
		o4 = oprd(ASUBBC, REGTMP, rd+1);
		o5 = opid(AMOVB, (v>>16)&0xff, REGTMP);
		o6 = oprd(ASUBBC, REGTMP, rd+2);
		o7 = opid(AMOVB, (v>>24)&0xff, REGTMP);
		o8 = oprd(ASUBBC, REGTMP, rd+3);
		break;
	case 64:		/* cmpw imm, r */
		diag("got to case 64");
		break;
	case 65:		/* movw $o(r), r big */
		if(v >= 0 && v < 64)
			diag("short offset in movw $o(r): %P", p);
		if(rs == NREG)
			rs = REGSP;
		even(p, 65, rs);
		notz(p, 65, rd);
		loreg(p, 65, rd);
		even(p, 65, rd);
		o1 = oprd(AMOVW, rs, rd);
		o2 = opid(AMOVB, v&0xff, REGTMP);
		o3 = oprd(AADDB, REGTMP, rd);
		o4 = opid(AMOVB, (v>>8)&0xff, REGTMP);
		o5 = oprd(AADDBC, REGTMP, rd+1);
		break;
	case 66:		/* movw $o(r), r */
		if(v < 0 || v >= 64)
			diag("long offset in movw $o(r): %P", p);
		if(rs == NREG)
			rs = REGSP;
		even(p, 66, rs);
		notz(p, 66, rd);
		even(p, 66, rd);
		o1 = oprd(AMOVW, rs, rd);
		o2 = opid(AMOVB, v&0xff, REGTMP);
		o3 = oprd(AADDB, REGTMP, rd);
		o4 = oprd(AADDBC, REGZERO, rd+1);
		break;
	case 67:		/* call z */
		o1 = opbrz(a);
		break;
	case 68:		/* cbi, sbic c,c */
		o1 = opid(a, p->from.offset, v);
		break;
	case 69:		/* load o(r) */
		if(v < 0 || v >= 64)
			diag("long offset in load o(r): %P", p);
		even(p, 69, rs);
		notz(p, 69, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = ldstxyz(0x8000, v, rd, REGZ);
		break;
	case 70:		/* store o(r) */
		if(v < 0 || v >= 64)
			diag("long offset in store o(r): %P", p);
		even(p, 70, rd);
		notz(p, 70, rs);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = ldstxyz(0x8200, v, rs, REGZ);
		break;
	case 71:		/* movw	0(r), r */
		even(p, 71, rs);
		even(p, 71, rd);
		notz(p, 71, rs);
		notz(p, 71, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = ldstxyz(0x9001, 0, rd, REGZ);
		o3 = ldstxyz(0x8000, 0, rd+1, REGZ);
		break;
	case 72:		/* movw o(r), r */
		v = -v;
		even(p, 72, rs);
		even(p, 72, rd);
		notz(p, 72, rs);
		notz(p, 72, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x9001, 0, rd, REGZ);
		o5 = ldstxyz(0x8000, 0, rd+1, REGZ);
		break;
	case 73:		/* movl	0(r), r */
		even(p, 73, rs);
		even(p, 73, rd);
		notz(p, 73, rs);
		notz(p, 73, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = ldstxyz(0x9001, 0, rd, REGZ);
		o3 = ldstxyz(0x9001, 0, rd+1, REGZ);
		o4 = ldstxyz(0x9001, 0, rd+2, REGZ);
		o5 = ldstxyz(0x8000, 0, rd+3, REGZ);
		break;
	case 74:		/* movl o(r), r */
		v = -v;
		even(p, 74, rs);
		even(p, 74, rd);
		notz(p, 74, rs);
		notz(p, 74, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x9001, 0, rd, REGZ);
		o5 = ldstxyz(0x9001, 0, rd+1, REGZ);
		o6 = ldstxyz(0x9001, 0, rd+2, REGZ);
		o7 = ldstxyz(0x8000, 0, rd+3, REGZ);
		break;
	case 75:		/* movw	r, 0(r) */
		even(p, 75, rs);
		even(p, 75, rd);
		notz(p, 75, rs);
		notz(p, 75, rd);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = ldstxyz(0x9201, 0, rs, REGZ);
		o3 = ldstxyz(0x8200, 0, rs+1, REGZ);
		break;
	case 76:		/* movw r, o(r) */
		v = -v;
		even(p, 76, rs);
		even(p, 76, rd);
		notz(p, 76, rs);
		notz(p, 76, rd);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x9201, 0, rs, REGZ);
		o5 = ldstxyz(0x8200, 0, rs+1, REGZ);
		break;
	case 77:		/* movl	r, 0(r) */
		even(p, 77, rs);
		even(p, 77, rd);
		notz(p, 77, rs);
		notz(p, 77, rd);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = ldstxyz(0x9201, 0, rs, REGZ);
		o3 = ldstxyz(0x9201, 0, rs+1, REGZ);
		o4 = ldstxyz(0x9201, 0, rs+2, REGZ);
		o5 = ldstxyz(0x8200, 0, rs+3, REGZ);
		break;
	case 78:		/* movl r, o(r) */
		v = -v;
		even(p, 78, rs);
		even(p, 78, rd);
		notz(p, 78, rs);
		notz(p, 78, rd);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x9201, 0, rs, REGZ);
		o5 = ldstxyz(0x9201, 0, rs+1, REGZ);
		o6 = ldstxyz(0x9201, 0, rs+2, REGZ);
		o7 = ldstxyz(0x8200, 0, rs+3, REGZ);
		break;
	case 79:		/* movb -(rs), rd / movb (rs)+, rd */
		xyzreg(p, 79, rs);
		switch(rs){
		case REGX:	a = 0x900d;	break;
		case REGY:	a = 0x9009;	break;
		case REGZ:	a = 0x9001;	break;
		}
		if(p->from.type == D_PREREG)
			a++;
		o1 = ldstxyz(a, 0, rd, rs);
		break;
	case 80:		/* movb rs, -(rd) / movb rs, (rd)+ */
		xyzreg(p, 80, rd);
		switch(rd){
		case REGX:	a = 0x920d;	break;
		case REGY:	a = 0x9209;	break;
		case REGZ:	a = 0x9201;	break;
		}
		if(p->to.type == D_PREREG)
			a++;
		o1 = ldstxyz(a, 0, rs, rd);
		break;
	case 81:		/* data in program memory */
		if(gen){
			pmdata(p);
			if(debug['a'])
				Bprint(&bso, " %.4lux: data\t%P\n", p->pc/2, p);
		}
		return regs;
	case 82:		/* movpm (r), r */
		notz(p, 82, rs);
		notz(p, 82, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = ldstxyz(0x9004, 0, rd, REGZ);
		break;
	case 83:		/* movpm o(r), r */
		v = -v;
		notz(p, 83, rs);
		notz(p, 83, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x9004, 0, rd, REGZ);
		break;
	case 84:		/* movpmw 0(r), r */
		even(p, 84, rs);
		even(p, 84, rd);
		notz(p, 84, rs);
		notz(p, 84, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = ldstxyz(0x9005, 0, rd, REGZ);
		o3 = ldstxyz(0x9004, 0, rd+1, REGZ);
		break;
	case 85:		/* movpmw O(r) */
		v = -v;
		even(p, 85, rs);
		even(p, 85, rd);
		notz(p, 85, rs);
		notz(p, 85, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x9005, 0, rd, REGZ);
		o5 = ldstxyz(0x9004, 0, rd+1, REGZ);
		break;
	case 86:		/* movpml 0(r), r */
		even(p, 86, rs);
		even(p, 86, rd);
		notz(p, 86, rs);
		notz(p, 86, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = ldstxyz(0x9005, 0, rd, REGZ);
		o3 = ldstxyz(0x9005, 0, rd+1, REGZ);
		o4 = ldstxyz(0x9005, 0, rd+2, REGZ);
		o5 = ldstxyz(0x9004, 0, rd+3, REGZ);
		break;
	case 87:		/* movpml O(r), r */
		v = -v;
		even(p, 87, rs);
		even(p, 87, rd);
		notz(p, 87, rs);
		notz(p, 87, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = ldstxyz(0x9005, 0, rd, REGZ);
		o5 = ldstxyz(0x9005, 0, rd+1, REGZ);
		o6 = ldstxyz(0x9005, 0, rd+2, REGZ);
		o7 = ldstxyz(0x9004, 0, rd+3, REGZ);
		break;
	case 88:		/* movpmb o(r), r */
		notz(p, 88, rs);
		notz(p, 88, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(AADDW, v, REGZ);
		o3 = ldstxyz(0x9004, 0, rd, REGZ);
		break;
	case 89:		/* movpmw o(r), r */
		notz(p, 89, rs);
		notz(p, 89, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(AADDW, v, REGZ);
		o3 = ldstxyz(0x9005, 0, rd, REGZ);
		o4 = ldstxyz(0x9004, 0, rd+1, REGZ);
		break;
	case 90:		/* movpml o(r), r */
		notz(p, 90, rs);
		notz(p, 90, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(AADDW, v, REGZ);
		o3 = ldstxyz(0x9005, 0, rd, REGZ);
		o4 = ldstxyz(0x9005, 0, rd+1, REGZ);
		o5 = ldstxyz(0x9005, 0, rd+2, REGZ);
		o6 = ldstxyz(0x9004, 0, rd+3, REGZ);
		break;
	case 91:		/* abr or call o(r) - o big */
		even(p, 91, rd);
		notz(p, 91, rd);
		if(v >= 0 || v < 64)
			diag("offset too small in abr o(r): %P", p);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = opid(ASUBB, v&0xff, REGZ);
		o3 = opid(ASUBBC, (v>>8)&0xff, REGZ+1);
		o4 = opbrz(a);
		break;
	case 92:		/* movw o(r), r - o small */
		even(p, 92, rs);
		even(p, 92, rd);
		notz(p, 92, rs);
		notz(p, 92, rd);
		if(v < 0 || v >= 63)
			diag("long offset in %P\n", p);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = ldstxyz(0x8000, v, rd, REGZ);
		o3 = ldstxyz(0x8000, v+1, rd+1, REGZ);
		break;
	case 93:		/* movl o(r), r - o small */
		notz(p, 93, rs);
		notz(p, 93, rd);
		o1 = oprd(AMOVW, rs, REGZ);
		o2 = opid(AADDW, v, REGZ);
		o3 = ldstxyz(0x9001, 0, rd, REGZ);
		o4 = ldstxyz(0x9001, 0, rd+1, REGZ);
		o5 = ldstxyz(0x9001, 0, rd+2, REGZ);
		o6 = ldstxyz(0x8000, 0, rd+3, REGZ);
		break;
	case 94:		/* movw r, o(r) - o small */
		even(p, 94, rs);
		even(p, 94, rd);
		notz(p, 94, rs);
		notz(p, 94, rd);
		if(v < 0 || v >= 63)
			diag("long offset in %P\n", p);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = ldstxyz(0x8200, v, rs, REGZ);
		o3 = ldstxyz(0x8200, v+1, rs+1, REGZ);
		break;
	case 95:		/* movl r, o(r) - o small */
		notz(p, 95, rs);
		notz(p, 95, rd);
		o1 = oprd(AMOVW, rd, REGZ);
		o2 = opid(AADDW, v, REGZ);
		o3 = ldstxyz(0x9201, 0, rs, REGZ);
		o4 = ldstxyz(0x9201, 0, rs+1, REGZ);
		o5 = ldstxyz(0x9201, 0, rs+2, REGZ);
		o6 = ldstxyz(0x8200, 0, rs+3, REGZ);
		break;
	case 96:		/* br sext => JMP */
		if(p->to.sym != nil && (p->to.sym->type == STEXT || p->to.sym->type == SLEAF))
			v /= 2;	/* possibly aclass should do this */
		o1 = oplbr(a, v, &o2);
		break;
	case 97:		/* movb 0(X|Y|Z), r */
		xyzreg(p, 97, rs);
		switch(rs){
		case REGX:	a = 0x900c;	break;
		case REGY:	a = 0x8008;	break;
		case REGZ:	a = 0x8000;	break;
		}
		o1 = ldstxyz(a, 0, rd, rs);
		break;
	case 98:		/* movb r, 0(X|Y|Z) */
		xyzreg(p, 98, rd);
		switch(rd){
		case REGX:	a = 0x920c;	break;
		case REGY:	a = 0x8208;	break;
		case REGZ:	a = 0x8200;	break;
		}
		o1 = ldstxyz(a, 0, rs, rd);
		break;
	case 99:		/* addb $c, r */
		hireg(p, 99, rd);
		v = -v;
		o1 = opid(ASUBB, v&0xff, rd);
		break;
	case 100:		/* addb $c, r */
		loreg(p, 100, rd);
		o1 = opid(AMOVB, v&0xff, REGTMP);
		o2 = oprd(AADDB, REGTMP, rd);
		break;
	case 101:		/* movpmb sext, r */
		notz(p, 101, rd);
		even(p, 101, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGZ);
		o2 = opid(AMOVB, (v>>8)&0xff, REGZ+1);
		o3 = ldstxyz(0x9004, 0, rd, REGZ);
		break;
	case 102:		/* movpmw sext, r */
		notz(p, 102, rd);
		even(p, 102, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGZ);
		o2 = opid(AMOVB, (v>>8)&0xff, REGZ+1);
		o3 = ldstxyz(0x9005, 0, rd, REGZ);
		o4 = ldstxyz(0x9004, 0, rd+1, REGZ);
		break;
	case 103:		/* movpml sext, r */
		notz(p, 103, rd);
		even(p, 103, rd);
		o1 = opid(AMOVB, (v>>0)&0xff, REGZ);
		o2 = opid(AMOVB, (v>>8)&0xff, REGZ+1);
		o3 = ldstxyz(0x9005, 0, rd, REGZ);
		o4 = ldstxyz(0x9005, 0, rd+1, REGZ);
		o5 = ldstxyz(0x9005, 0, rd+2, REGZ);
		o6 = ldstxyz(0x9004, 0, rd+3, REGZ);
		break;
	case 104:		/* save */
		if(gen){
			a = 0;
			rs = regmask();
			for(rd = 31; rd >= 0; rd--){
				if(rs&(1<<31)){
					o1 = opd(APUSHB, rd);
					if(debug['a'])
						Bprint(&bso, " %.4lux: %.4lux\t\tPUSHB	,R%d\n", p->pc/2+a, o1, rd);
					hputl(o1);
					a++;
				}
				rs <<= 1;
			}
			o1 = opid(AIN, SREG, REGTMP);
			o2 = opd(APUSHB, REGTMP);
			if(debug['a']){
				Bprint(&bso, " %.4lux: %.4lux\t\tIN	$%d,R%d\n", p->pc/2+a, o1, SREG, REGTMP);
				Bprint(&bso, " %.4lux: %.4lux\t\tPUSHB	,R%d\n", p->pc/2+a+1, o2, REGTMP);
			}
			hputl(o1);
			hputl(o2);
		}
		return regs;
	case 105:		/* restore */
		if(gen){
			o1 = opd(APOPB, REGTMP);
			o2 = opid(AOUT, SREG, REGTMP);
			if(debug['a']){
				Bprint(&bso, " %.4lux: %.4lux\t\tPOPB	,R%d\n", p->pc/2, o1, REGTMP);
				Bprint(&bso, " %.4lux: %.4lux\t\tOUT	$%d,R%d\n", p->pc/2+1, o2, SREG, REGTMP);
			}
			hputl(o1);
			hputl(o2);
			a = 2;
			rs = regmask();
			for(rd = 0; rd <= 31; rd++){
				if(rs&1){
					o1 = opd(APOPB, rd);
					if(debug['a'])
						Bprint(&bso, " %.4lux: %.4lux\t\tPOPB	,R%d\n", p->pc/2+a, o1, rd);
					hputl(o1);
					a++;
				}
				rs >>= 1;
			}
		}
		return regs;
	}
	if(!gen)
		return regs;
	v = p->pc/2;
	switch(o->size) {
	default:
		if(o->type != 0)
			diag("bad size %d in asmout: %P", o->size, p);
		if(debug['a'])
			Bprint(&bso, " %.4lux:\t\t%P\n", v, p);
		break;
	case 2:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux\t%P\n", v, o1, p);
		hputl(o1);
		break;
	case 4:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux%P\n", v, o1, o2, p);
		hputl(o1);
		hputl(o2);
		break;
	case 6:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux %.4lux%P\n", v, o1, o2, o3, p);
		hputl(o1);
		hputl(o2);
		hputl(o3);
		break;
	case 8:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux %.4lux %.4lux%P\n", v, o1, o2, o3, o4, p);
		hputl(o1);
		hputl(o2);
		hputl(o3);
		hputl(o4);
		break;
	case 10:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux %.4lux %.4lux %.4lux%P\n", v, o1, o2, o3, o4, o5, p);
		hputl(o1);
		hputl(o2);
		hputl(o3);
		hputl(o4);
		hputl(o5);
		break;
	case 12:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux%P\n", v, o1, o2, o3, o4, o5, o6, p);
		hputl(o1);
		hputl(o2);
		hputl(o3);
		hputl(o4);
		hputl(o5);
		hputl(o6);
		break;
	case 14:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux%P\n", v, o1, o2, o3, o4, o5, o6, o7, p);
		hputl(o1);
		hputl(o2);
		hputl(o3);
		hputl(o4);
		hputl(o5);
		hputl(o6);
		hputl(o7);
		break;
	case 16:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux%P\n", v, o1, o2, o3, o4, o5, o6, o7, o8, p);
		hputl(o1);
		hputl(o2);
		hputl(o3);
		hputl(o4);
		hputl(o5);
		hputl(o6);
		hputl(o7);
		hputl(o8);
		break;
	case 18:
		if(debug['a'])
			Bprint(&bso, " %.4lux: %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux %.4lux%P\n", v, o1, o2, o3, o4, o5, o6, o7, o8, o9, p);
		hputl(o1);
		hputl(o2);
		hputl(o3);
		hputl(o4);
		hputl(o5);
		hputl(o6);
		hputl(o7);
		hputl(o8);
		hputl(o9);
		break;
	}
	return regs;
}

static int
regmask(void)
{
	/* REGTMP but not REGZERO */
	return (curtext->from.sym->regused|1<<REGTMP)&~(3<<REGZERO);
}

static int
bits(int n)
{
	n = (n & 0x55555555) + (n>>1 & 0x55555555);
	n = (n & 0x33333333) + (n>>2 & 0x33333333);
	n = (n & 0x0F0F0F0F) + (n>>4 & 0x0F0F0F0F);
	n = (n & 0x00FF00FF) + (n>>8 & 0x00FF00FF);
	n = (n & 0x0000FFFF) + (n>>16 & 0x0000FFFF);
	return n;
}

int
isize(Prog *p)
{
	switch(p->as){
	case ASAVE:
	case ARESTORE:
		return 2*(bits(regmask())+2);
	}
	diag("bad instruction %P in isize", p);
	return 0;
}

/*
 * words
 *	ABREAK	0x9598
 *	ANOP	0x0000	ANOP is special to zl
 *	ARET	0x9508	stack popped by 2 or 3 bytes
 *	ARETI	0x9518	stack popped by 2 or 3 bytes
 *	ASLEEP	0x9588
 *	ASPM	0x95E8
 *	AWDR	0x95A8
 * call/jump
 *	ACALL	0x940E	absolute	32-bit with odd & variable format (see p. 47)
 *	ACALL	0x9519	(EIND:Z)	EICALL
 *	ACALL	0x9509	(Z)	ICALL
 *	AJMP	0x9419	(EIND:Z)	EIJMP
 *	AJMP	0x9409	(Z)	IJMP
 *	AJMP	0x940C	absolute	32-bit with variable format (see p. 82)
 *	ACALL	0xD000	+-2kw	pc-relative RCALL p. 108
 *	AJMP	0xC000	+-2kw	pc-relative RJMP p. 111
 * special move
 *	AMOVB	0x900C	(X),Rd	LD
 *	AMOVB	0x900D	(X)+, Rd
 *	AMOVB	0x900E	-(X), Rd
 *	AMOVB	0x8008	(Y), Rd	LDD, same as q(Y) with q=0, p. 85
 *	AMOVB	0x9009	(Y)+, Rd
 *	AMOVB	0x900A	-(Y), Rd
 *	AMOVB	0x8008	q(Y), Rd
 *	AMOVB	0x8000	(Z), Rd	LDD, same as q(Z) with q=0, p. 87
 *	AMOVB	0x9001	(Z)+, Rd
 *	AMOVB	0x9002	-(Z), Rd
 *	AMOVB	0x8000	q(Z), Rd
 *	AMOVB	0x9000	16-bitaddr, Rd	32-bit instruction, LDS p. 90
 *	AMOVB	0x920C	Rr, (X)	ST	p. 137
 *	AMOVB	0x920D	Rr, (X)+
 *	AMOVB	0x920E	Rr, -(X)
 *	AMOVB	0x8208	Rr, (Y)	STD	p. 139
 *	AMOVB	0x9209	Rr, (Y)+
 *	AMOVB	0x920A	Rr, -(Y)
 *	AMOVB	0x8208	Rr, q(Y)
 *	AMOVB	0x8200	Rr, (Z)
 *	AMOVB	0x9201	Rr, (Z)+
 *	AMOVB	0x9202	Rr, -(Z)
 *	AMOVB	0x8200	Rr, q(Z)
 *	AMOVB	0x9200	Rr, 16-bitaddr	32-bit instruction, STS, p. 143
 *	ALPM	0x95B8	(RAMPZ:Z), R0	ELPM	(d&1F)<<4
 *	ALPM	0x9006	(RAMPZ:Z), Rd	ELPM
 *	ALPM	0x9007	(RAMPZ:Z)+, Rd	ELPM
 *	ALPM	0x95C8	(Z), R0
 *	ALPM	0x9004	(Z), Rd
 *	ALPM	0x9005	(Z)+, Rd
 */

/*
 * SREG bits:
 *	C=0, Z=1, N=2, V=3, S=4, H=5, T=6, I=7
 */

long
op(int a)
{
	switch(a) {
	case ABREAK:	return 0x9598;
	case ANOP:	return 0x0000;
	case ASLEEP:	return 0x9588;
	case AWDR:	return 0x95A8;
	}
	diag("bad op() opcode %A", a);
	return 0;
}

long
opi(int a, int i)
{
	switch(a) {
	case ABCLR:	return 0x9488 | ((i&7)<<4);	/* clear bit i in SREG */
	case ABSET:	return 0x9408 | ((i&7)<<4);	/* set bit i in SREG */
	}
	diag("bad imm opcode %A", a);
	return 0;
}

#define	OPD(v, d)	((v) | (((d)&0x1F)<<4))
#define	OPBRB(v, k, s)	((v) | (((k)&0x7F)<<3) | ((s)&7))
#define	OPRD(v, r, d)	((v) | (((r)&0x10)<<5) | (((d)&0x1F)<<4) | ((r)&0xF))
#define	OPIDB(v, i, d)	((v) | (((i)&0xF0)<<4) | (((d)&0xF)<<4) | ((i)&0xF))
#define	OPIDW(v, i, d)	((v) | (((i)&0x30)<<2) | (((d)&6)<<3) | ((i)&0xF))
#define	OPIO(v, i, d)	((v) | (((i)&0x30)<<5) | (((d)&0x1F)<<4) | ((i)&0xF))
#define	OPIOB(v, i, d)	((v) | (((d)&0x1F)<<3) | ((i)&7))
#define	OPSKIP(v, i, d)	((v) | (((d)&0x1F)<<4) | ((i)&7))

long
opd(int a, int d)
{
	regs |= 1<<d;
	switch(a) {
	case AASRB:	return OPD(0x9405, d);
	case ACLRB:	return OPRD(0x2400, d, d);	/* EOR d, d */
	case ACOMB:	return OPD(0x9400, d);
	case ADEC:	return OPD(0x940A, d);
	case AINC:	return OPD(0x9403, d);
	case ALSLB:	return OPRD(0x0C00, d, d);	/* ADD d, d */
	case ALSRB:	return OPD(0x9406, d);
	case ANEGB:	return OPD(0x9401, d);
	case APOPB:	return OPD(0x900F, d);
	case APUSHB:	return OPD(0x920F, d);
	case AROLB:	return OPRD(0x1C00, d, d);	/* ADC d, d */
	case ARORB:	return OPD(0x9407, d);
	// case ASERB:	return OPIDB(0xE000, 255, d);	/* LDI 255, d */
	case ASWAP:	return OPD(0x9402, d);
	case ATST:	return OPRD(0x2000, d, d);	/* AND d, d */
	}
	diag("bad opd opcode %A", a);
	return 0;
}

long
opbr(int a, int k)
{
	switch(a){
	case ABR:		return 0xC000 | (k & 0xfff);
	case ACALL:	return 0xD000 | (k & 0xfff);
	}
	diag("bad opbr opcode %A", a);
	return 0;
}

long
oplbr(int a, int k, long *o)
{
	*o = k & 0xffff;
	switch(a){
	case AVECTOR:
	case ABR:		return 0x940C | ((k>>13) & 0x1f0) | ((k>>16) & 1);
	case ACALL:	return 0x940E | ((k>>13) & 0x1f0) | ((k>>16) & 1);
	}
	diag("bad oplbr opcode %A", a);
	return 0;
}

long
opbrz(int a)
{
	regs |= 3<<REGZ;
	switch(a){
	case ABR:		return 0x9409;
	case	ACALL:	return 0x9509;
	}
	diag("bad opbrz opcode %A", a);
	return 0;
}
	
long
opbrbs(int a, int k, int s)
{
	switch(a) {
	case ABRBC:	return OPBRB(0xF400, k, s);
	case ABRBS:	return OPBRB(0xF000, k, s);
	}
	diag("bad opbrbs opcode %A", a);
	return 0;
}

long
opbrb(int a, int k)
{
	switch(a) {
	case ABCC:	return OPBRB(0xF400, k, 0);
	case ABCS:	return OPBRB(0xF000, k, 0);
	case ABEQ:	return OPBRB(0xF000, k, 1);
	case ABGE:	return OPBRB(0xF400, k, 4);
	case ABHC:	return OPBRB(0xF400, k, 5);
	case ABHS:	return OPBRB(0xF000, k, 5);
	case ABID:	return OPBRB(0xF400, k, 7);
	case ABIE:		return OPBRB(0xF000, k, 7);
	case ABLO:	return OPBRB(0xF000, k, 0);
	case ABLT:	return OPBRB(0xF000, k, 4);
	case ABMI:	return OPBRB(0xF000, k, 2);
	case ABNE:	return OPBRB(0xF400, k, 1);
	case ABPL:	return OPBRB(0xF400, k, 2);
	case ABSH:	return OPBRB(0xF400, k, 0);
	case ABTC:	return OPBRB(0xF400, k, 6);
	case ABTS:	return OPBRB(0xF000, k, 6);
	case ABVC:	return OPBRB(0xF400, k, 3);
	case ABVS:	return OPBRB(0xF000, k, 3);
	}
	diag("bad opbrb opcode %A", a);
	return 0;
}

long
oprd(int a, int r, int d)
{
	int o;

	regs |= 1<<r;
	regs |= 1<<d;
	if(a == AMOVW){
		regs |= 1<<(r+1);
		regs |= 1<<(d+1);
	}
	switch(a){
	case AFMULSB:
	case AFMULSUB:
	case AFMULUB:
	case AMULB:
	case AMULUB:
	case AMULSUB:
		regs |= 3<<0;
	}
	switch(a) {
	case AADDBC:	o = 0x1C00; break;
	case AADDB:	o = 0x0C00; break;
	case AANDB:	o = 0x2000; break;
	case AEORB:	o = 0x2400; break;	/* when r==d used for CLRB Rd or MOVB $0, Rd*/
	case ACMPB:	o = 0x1400; break;
	case ACMPBC:	o = 0x0400; break;
	case ACMPSE:	o = 0x1000; break;
	case AFMULUB:	o = 0x0308; r &= 7; d &= 7; break;	 /* r, d, limited to [16,23] */
	case AFMULSB:	o = 0x0380; r &= 7; d &= 7; break;
	case AFMULSUB:	o = 0x0388; r &= 7; d &= 7; break;
	case AMOVB:	o = 0x2C00; break;
	case AMOVW:	return 0x0100 | (((d>>1)&0xF)<<4) | ((r>>1)&0xF);
	case AMULUB:	o = 0x9C00; break;
	case AMULB:	o = 0x0200; r &= 0xF; d &= 0xF; break;	/* r, d, limited to [16,31] */
	case AMULSUB:	o = 0x0300; r &= 7; d &= 7; break;	/* r, d, limited to [16,23] */
	case AORB:	o = 0x2800; break;
	case ASUBBC:	o = 0x0800; break;
	case ASUBB:	o = 0x1800; break;
	default:
		diag("bad r/r opcode %A", a);
		o = 0;
	}
	return OPRD(o, r, d);
}

long
opid(int a, int i, int d)
{
	regs |= 1<<d;
	if(a == AADDW || a == ASUBW)
		regs |= 1<<(d+1);
	switch(a) {
	case AADDW:	return OPIDW(0x9600, i, d);	/* ADIW */
	case AANDB:	return OPIDB(0x7000, i, d);	/* ANDI */
	case ABLD:	return 0xF800 | ((d&0x1F)<<4) | (i&7);
	case ABST:	return 0xFA00 | ((d&0x1F)<<4) | (i&7);	/* bit store from reg to SREG */
	case ACBI:	return OPIOB(0x9800, i, d);
	case ACMPB:	return OPIDB(0x3000, i, d);	/* CPI */
	case AIN:		return OPIO(0xB000, i, d);
	case AMOVB:	return OPIDB(0xE000, i, d);	/* LDI */
	case AORB:	return OPIDB(0x6000, i, d);	/* ORI */
	case AOUT:	return OPIO(0xB800, i, d);

	case ASUBBC:	return OPIDB(0x4000, i, d);	/* SBCI */
	case ASBI:		return OPIOB(0x9A00, i, d);	/* SBI */
	case ASBIC:	return OPIOB(0x9900, i, d);	/* SBIC */
	case ASBIS:	return OPIOB(0x9B00, i, d);	/* SBIS */
	case ASUBW:	return OPIDW(0x9700, i, d);	/* SBIW */
	case ASBRC:	return OPSKIP(0xFC00, i, d);
	case ASBRS:	return OPSKIP(0xFE00, i, d);
	case ASUBB:	return OPIDB(0x5000, i, d);	/* SUBI */
	}
	diag("bad i/r opcode %A", a);
	return 0;
}

long
ldstdata(int a, int r)
{
	regs |= 1<<r;
	return a | (r<<4);
}

long
ldstxyz(int a, int k, int r, int xyz)
{
	regs |= 1<<r;
	regs |= 3<<xyz;
	return a | (r<<4) | ((k&0x20)<<8) | ((k&0x18)<<7) | (k&0x7);
}
