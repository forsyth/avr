#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "../zc/z.out.h"
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = 'z';
	thestring = "avr";
	memset(debug, 0, sizeof(debug));
	cinit();
	outfile = 0;
	include[ninclude++] = ".";
	ARGBEGIN {
	default:
		c = ARGC();
		if(c >= 0 || c < sizeof(debug))
			debug[c] = 1;
		break;

	case 'o':
		outfile = ARGF();
		break;

	case 'D':
		p = ARGF();
		if(p)
			Dlist[nDlist++] = p;
		break;

	case 'I':
		p = ARGF();
		setinclude(p);
		break;
	} ARGEND
	if(*argv == 0) {
		print("usage: %ca [-options] file.s\n", thechar);
		errorexit();
	}
	if(argc > 1 && systemtype(Windows)){
		print("can't assemble multiple files on windows\n");
		errorexit();
	}
	if(argc > 1 && !systemtype(Windows)) {
		nproc = 1;
		if(p = getenv("NPROC"))
			nproc = atol(p);	/* */
		c = 0;
		nout = 0;
		for(;;) {
			while(nout < nproc && argc > 0) {
				i = myfork();
				if(i < 0) {
					i = mywait(&status);
					if(i < 0)
						errorexit();
					if(status)
						c++;
					nout--;
					continue;
				}
				if(i == 0) {
					print("%s:\n", *argv);
					if(assemble(*argv))
						errorexit();
					exits(0);
				}
				nout++;
				argc--;
				argv++;
			}
			i = mywait(&status);
			if(i < 0) {
				if(c)
					errorexit();
				exits(0);
			}
			if(status)
				c++;
			nout--;
		}
	}
	if(assemble(argv[0]))
		errorexit();
	exits(0);
}

int
assemble(char *file)
{
	char ofile[100], incfile[20], *p;
	int i, of;

	strcpy(ofile, file);
	p = utfrrune(ofile, pathchar());
	if(p) {
		include[0] = ofile;
		*p++ = 0;
	} else
		p = ofile;
	if(outfile == 0) {
		outfile = p;
		if(outfile){
			p = utfrrune(outfile, '.');
			if(p)
				if(p[1] == 's' && p[2] == 0)
					p[0] = 0;
			p = utfrune(outfile, 0);
			p[0] = '.';
			p[1] = thechar;
			p[2] = 0;
		} else
			outfile = "/dev/null";
	}
	p = getenv("INCLUDE");
	if(p) {
		setinclude(p);
	} else {
		if(systemtype(Plan9)) {
			sprint(incfile,"/%s/include", thestring);
			setinclude(strdup(incfile));
		}
	}

	of = mycreat(outfile, 0664);
	if(of < 0) {
		yyerror("%ca: cannot create %s", thechar, outfile);
		errorexit();
	}
	Binit(&obuf, of, OWRITE);

	pass = 1;
	pinit(file);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	if(nerrors) {
		cclean();
		return nerrors;
	}

	pass = 2;
	outhist();
	pinit(file);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	cclean();
	return nerrors;
}

struct
{
	char	*name;
	ushort	type;
	ushort	value;
} itab[] =
{
	"SP",		LSP,	D_AUTO,
	"SB",		LSB,	D_EXTERN,
	"FP",		LFP,	D_PARAM,
	"PC",		LPC,	D_BRANCH,

	"W",		LREG,	D_W,
	"WH",		LREG,	D_W+1,
	"X",		LREG,	D_X,
	"XH",		LREG,	D_X+1,
	"Y",		LREG,	D_Y,
	"YH",		LREG,	D_Y+1,
	"Z",		LREG,	D_Z,
	"ZH",		LREG,	D_Z+1,

	"R",		LR,	0,
	"R0",		LREG,	0,
	"R1",		LREG,	1,
	"R2",		LREG,	2,
	"R3",		LREG,	3,
	"R4",		LREG,	4,
	"R5",		LREG,	5,
	"R6",		LREG,	6,
	"R7",		LREG,	7,
	"R8",		LREG,	8,
	"R9",		LREG,	9,
	"R10",		LREG,	10,
	"R11",		LREG,	11,
	"R12",		LREG,	12,
	"R13",		LREG,	13,
	"R14",		LREG,	14,
	"R15",		LREG,	15,
	"R16",		LREG,	16,
	"R17",		LREG,	17,
	"R18",		LREG,	18,
	"R19",		LREG,	19,
	"R20",		LREG,	20,
	"R21",		LREG,	21,
	"R22",		LREG,	22,
	"R23",		LREG,	23,
	"R24",		LREG,	24,
	"R25",		LREG,	25,
	"R26",		LREG,	26,
	"R27",		LREG,	27,
	"R28",		LREG,	28,
	"R29",		LREG,	29,
	"R30",		LREG,	30,
	"R31",		LREG,	31,

	"ADDB",	LADD, AADDB,	/* ADD */
	"ADDBC",	LADD, AADDBC,	/* ADC */
	"ADDL",	LADD, AADDL,
	"ADDW",	LADD, AADDW,	/* ADIW, macro add word */
	"ANDB",	LADD, AANDB,	/* includes ANDI */
	"ANDL",	LADD, AANDL,
	"ANDW",	LADD, AANDW,
	"ASRB",	LCOM, AASRB,
	"ASRL",	LCOM, AASRL,
	"ASRW",	LCOM, AASRW,
	"BCLR",	LBCLR, ABCLR,
	"BLD",	LBST, ABLD,
	"BSET",	LBCLR, ABSET,
	"BST",	LBST, ABST,
	"CALL",	LBRA, ACALL,	/* includes RCALL, ICALL, EICALL, and CALL */
	"CBI",	LSBIC, ACBI,
	"CBRB",	LCBR, ACBRB,
	"CBRL",	LCBR, ACBRL,
	"CBRW",	LCBR, ACBRW,
	"CLRB",	LCOM, ACLRB,
	"CLRL",	LCOM, ACLRL,
	"CLRW",	LCOM, ACLRW,
	"CMPB",	LADD, ACMPB,		/* includes CPI */
	"CMPBC",	LEOR, ACMPBC,
	"CMPL",	LADD, ACMPL,
	"CMPW",	LADD, ACMPW,
	"COMB",	LCOM,	ACOMB,
	"COML",	LCOM,	ACOML,
	"COMW",	LCOM,	ACOMW,
	"CMPSE",		LEOR, ACMPSE,
	"DEC",	LCOM,	ADEC,
	"EORB",	LEOR,	AEORB,
	"EORL",	LEOR,	AEORL,
	"EORW",	LEOR,	AEORW,
	"FMULSB",	LEOR,	AFMULSB,
	"FMULSUB",	LEOR,	AFMULSUB,
	"FMULUB",	LEOR,	AFMULUB,
	"IN",			LIN, AIN,
	"INC",	LCOM,	AINC,
	"LSLB",		LCOM, ALSLB,
	"LSLL",		LCOM, ALSLL,
	"LSLW",		LCOM, ALSLW,
	"LSRB",		LCOM, ALSRB,
	"LSRL",		LCOM, ALSRL,
	"LSRW",		LCOM, ALSRW,
	"MOVB",	LMOVB,	AMOVB,	/* MOV, LD, LDD, LDI, LDS, ST, STD, STS */
	"MOVBL",	LMOVW,	AMOVBL,
	"MOVBW",	LMOVW,	AMOVBW,
	"MOVBZL",	LMOVW,	AMOVBZL,
	"MOVBZW",	LMOVW,	AMOVBZW,
	"MOVL",	LMOVW,	AMOVL,
	"MOVPM",	LMOVB,	AMOVPM,
	"MOVW",	LMOVW,	AMOVW,
	"MOVWL",	LMOVW,	AMOVWL,
	"MOVWZL",	LMOVW,	AMOVWZL,
	"MULB",	LADD,	AMULB,
	"MULSUB",	LADD,	AMULSUB,
	"MULBUS",	LADD,	AMULSUB,
	"MULBU",	LADD,	AMULUB,
	"MULUB",	LADD,	AMULUB,
	"NEGB",	LCOM,	ANEGB,
	"NEGL",	LCOM,	ANEGL,
	"NEGW",	LCOM,	ANEGW,
	"ORB",	LADD,	AORB,	/* includes ORI */
	"ORL",	LADD,	AORL,
	"ORW",	LADD,	AORW,
	"OUT",		LOUT, AOUT,
	"POPB",		LCOM, APOPB,
	"POPL",		LCOM, APOPL,
	"POPW",		LCOM, APOPW,
	"PUSHB",		LCOM, APUSHB,
	"PUSHL",		LCOM, APUSHL,
	"PUSHW",		LCOM, APUSHW,
	"RET",		LRETRN, ARET,
	"RETI",		LRETRN, ARETI,
	"ROLB",		LCOM, AROLB,
	"ROLL",		LCOM, AROLL,
	"ROLW",		LCOM, AROLW,
	"RORB",		LCOM, ARORB,
	"RORL",		LCOM, ARORL,
	"RORW",		LCOM, ARORW,
	"SBI",		LSBIC, ASBI,
	"SBIC",		LSBIC, ASBIC,
	"SBIS",		LSBIC, ASBIS,
	"SBRB",	LCBR, ASBRB,
	"SBRL",	LCBR, ASBRL,
	"SBRW",	LCBR, ASBRW,
	"SBRC",		LBST, ASBRC,
	"SBRS",		LBST, ASBRS,
	"SUBB",	LADD,	ASUBB,
	"SUBBC",	LADD,	ASUBBC,	/* includes SBCI */
	"SUBL",	LADD,	ASUBL,
	"SUBW",	LADD,	ASUBW,	/* includes SBIW */
	"SWAP",		LCOM, ASWAP,
	"TST",	LCOM,	ATST,

	/* BRBS, BRBC: */
	/* C (carry), Z (zero), N (negative), V (overflow), S (N ^ V), H (half carry), T (transfer), I (interrupt) */
	"JMP",		LBRA,	ABR,	/* synonym, for now */
	"BR",			LBRA,	ABR,
	"BEQ",		LBRA, ABEQ,
	"BNE",		LBRA, ABNE,
	"BCS",		LBRA, ABCS,
	"BCC",		LBRA, ABCC,
	"BSH",		LBRA, ABSH,	/* same or higher */
	"BSL",		LBRA, ABSL,	/* same or lower */
	"BLO",		LBRA, ABLO,
	"BMI",		LBRA, ABMI,
	"BPL",		LBRA, ABPL,
	"BGE",		LBRA, ABGE,
	"BLT",		LBRA, ABLT,
	"BHC",		LBRA, ABHC,
	"BHS",		LBRA, ABHS,
	"BTC",		LBRA, ABTC,
	"BTS",		LBRA, ABTS,
	"BVC",		LBRA, ABVC,
	"BVS",		LBRA, ABVS,
	"BIE",		LBRA, ABIE,
	"BID",		LBRA, ABID,
	"BGT",		LBRA, ABGT,
	"BHI",		LBRA, ABHI,
	"BLE",		LBRA, ABLE,
	"BRBS",		LBRAS,	ABRBS,
	"BRBC",		LBRAS,	ABRBC,

	"BREAK",		LRETRN, ABREAK,
	"NOP",		LRETRN, ANOP,
	"SLEEP",		LRETRN, ASLEEP,
	"WDR",		LRETRN, AWDR,
	"SAVE",		LRETRN,	ASAVE,
	"RESTORE",	LRETRN,	ARESTORE,

	"DATA",		LDATA, ADATA,
	"END",		LEND, AEND,
	"GLOBL",		LTEXT, AGLOBL,
	"PM",		LTEXT, APM,
	"TEXT",		LTEXT, ATEXT,
	/* "NOP",		LNOP, ANOP, */	/* use AWORD for real NOP */
	"VECTOR",	LBRA,	AVECTOR,

	"WORD",		LWORD, AWORD,
	"SCHED",	LSCHED, 0,
	"NOSCHED",	LSCHED,	0x80,

	/* SEC etc are done using #define to BSET/BCLR */

	0
};

void
cinit(void)
{
	Sym *s;
	int i;

	nullgen.sym = S;
	nullgen.offset = 0;
	nullgen.type = D_NONE;
	nullgen.name = D_NONE;
	nullgen.reg = NREG;
	nullgen.xreg = NREG;
	if(FPCHIP)
		nullgen.dval = 0;
	for(i=0; i<sizeof(nullgen.sval); i++)
		nullgen.sval[i] = 0;

	nerrors = 0;
	iostack = I;
	iofree = I;
	peekc = IGN;
	nhunk = 0;
	for(i=0; i<NHASH; i++)
		hash[i] = S;
	for(i=0; itab[i].name; i++) {
		s = slookup(itab[i].name);
		s->type = itab[i].type;
		s->value = itab[i].value;
	}
	ALLOCN(pathname, 0, 100);
	if(getwd(pathname, 99) == 0) {
		ALLOCN(pathname, 100, 900);
		if(getwd(pathname, 999) == 0)
			strcpy(pathname, "/???");
	}
}

void
syminit(Sym *s)
{

	s->type = LNAME;
	s->value = 0;
}

void
cclean(void)
{

	outcode(AEND, &nullgen, NREG, &nullgen);
	Bflush(&obuf);
}

void
zname(char *n, int t, int s)
{

	Bputc(&obuf, ANAME);
	Bputc(&obuf, t);	/* type */
	Bputc(&obuf, s);	/* sym */
	while(*n) {
		Bputc(&obuf, *n);
		n++;
	}
	Bputc(&obuf, 0);
}

void
zaddr(Gen *a, int s)
{
	long l;
	int i;
	char *n;
	Ieee e;

	Bputc(&obuf, a->type);
	Bputc(&obuf, a->reg);
	Bputc(&obuf, s);
	Bputc(&obuf, a->name);
	switch(a->type) {
	default:
		print("unknown type %d\n", a->type);
		exits("arg");

	case D_NONE:
	case D_REG:
	case D_SREG:
	case D_PORT:
	case D_PREREG:
	case D_POSTREG:
	case D_RAMPD:
	case D_RAMPY:
	case D_RAMPZ:
		break;

	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		break;

	case D_SCONST:
		n = a->sval;
		for(i=0; i<NSNAME; i++) {
			Bputc(&obuf, *n);
			n++;
		}
		break;

	case D_FCONST:
		ieeedtod(&e, a->dval);
		Bputc(&obuf, e.l);
		Bputc(&obuf, e.l>>8);
		Bputc(&obuf, e.l>>16);
		Bputc(&obuf, e.l>>24);
		Bputc(&obuf, e.h);
		Bputc(&obuf, e.h>>8);
		Bputc(&obuf, e.h>>16);
		Bputc(&obuf, e.h>>24);
		break;
	}
}

int
outsim(Gen *g)
{
	Sym *s;
	int sno, t;

	s = g->sym;
	if(s == S)
		return 0;
	sno = s->sym;
	if(sno < 0 || sno >= NSYM)
		sno = 0;
	t = g->name;
	if(h[sno].type == t && h[sno].sym == s)
		return sno;
	zname(s->name, t, sym);
	s->sym = sym;
	h[sym].sym = s;
	h[sym].type = t;
	sno = sym;
	sym++;
	if(sym >= NSYM)
		sym = 1;
	return sno;
}

void
outcode(int a, Gen *g1, int reg, Gen *g2)
{
	int sf, st;

	if(a != AGLOBL && a != ADATA && a != APM)
		pc++;
	if(pass == 1)
		return;
	if(g1->xreg != NREG) {
		if(reg != NREG || g2->xreg != NREG)
			yyerror("bad addressing modes");
		reg = g1->xreg;
	} else
	if(g2->xreg != NREG) {
		if(reg != NREG)
			yyerror("bad addressing modes");
		reg = g2->xreg;
	}
	do {
		sf = outsim(g1);
		st = outsim(g2);
	} while(sf != 0 && st == sf);
	Bputc(&obuf, a);
	Bputc(&obuf, reg|nosched);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, sf);
	zaddr(g2, st);
}

void
outgcode(int a, Gen *g1, int reg, Gen *g2, Gen *g3)
{
	int s1, s2, s3, flag; 

	if(a != AGLOBL && a != ADATA && a != APM)
		pc++;
	if(pass == 1)
		return;
	do {
		s1 = outsim(g1);
		s2 = outsim(g2);
		s3 = outsim(g3);
	} while(s1 && (s2 && s1 == s2 || s3 && s1 == s3) || s2 && (s3 && s2 == s3));
	flag = 0;
	if(g2->type != D_NONE)
		flag = 0x40;	/* flags extra operand */
	Bputc(&obuf, a);
	Bputc(&obuf, reg | nosched | flag);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, s1);
	if(flag)
		zaddr(g2, s2);
	zaddr(g3, s3);
}

void
outhist(void)
{
	Gen g;
	Hist *h;
	char *p, *q, *op;
	int n;

	g = nullgen;
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		op = 0;
		if(p && p[0] != '/' && h->offset == 0 && pathname && pathname[0] == '/') {
			op = p;
			p = pathname;
		}
		while(p) {
			q = strchr(p, '/');
			if(q) {
				n = q-p;
				if(n == 0)
					n = 1;	/* leading "/" */
				q++;
			} else {
				n = strlen(p);
				q = 0;
			}
			if(n) {
				Bputc(&obuf, ANAME);
				Bputc(&obuf, D_FILE);	/* type */
				Bputc(&obuf, 1);	/* sym */
				Bputc(&obuf, '<');
				Bwrite(&obuf, p, n);
				Bputc(&obuf, 0);
			}
			p = q;
			if(p == 0 && op) {
				p = op;
				op = 0;
			}
		}
		g.offset = h->offset;

		Bputc(&obuf, AHISTORY);
		Bputc(&obuf, 0);
		Bputc(&obuf, h->line);
		Bputc(&obuf, h->line>>8);
		Bputc(&obuf, h->line>>16);
		Bputc(&obuf, h->line>>24);
		zaddr(&nullgen, 0);
		zaddr(&g, 0);
	}
}

int
xyzreg(int r)
{
	return r == D_X || r == D_Y || r == D_Z;
}

/*

void
praghjdicks(void)
{
	while(getnsc() != '\n')
		;
}

void
pragvararg(void)
{
	while(getnsc() != '\n')
		;
}

void
pragfpround(void)
{
	while(getnsc() != '\n')
		;
}

*/

#include "../cc/lexbody"
#include "../cc/macbody"
#include "../cc/compat"
