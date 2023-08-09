#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../zc/z.out.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

struct	Adr
{
	union
	{
		long	u0offset;
		char	u0sval[NSNAME];
		Ieee	u0ieee;
	}u0;
	Sym	*sym;
	Auto	*autom;
	char	type;
	uchar	reg;
	char	name;
	char	class;
};

#define	offset	u0.u0offset
#define	sval	u0.u0sval
#define	cond	u0.u0cond
#define	ieee	u0.u0ieee

struct	Prog
{
	Adr	from;
	Adr	to;
	Prog	*forwd;
	Prog	*pcond;
	Prog	*link;
	long	pc;
	short	line;
	short	mark;
	short	optab;		/* could be uchar */
	uchar	as;
	uchar	flag;
	long	size;
};
struct	Sym
{
	char	*name;
	short	type;
	short	version;
	short	fnptr;
	short	frame;
	long	value;
	long	regused;
	Sym	*link;
};
struct	Autom
{
	Sym	*asym;
	Auto	*link;
	long	aoffset;
	short	type;
};
struct	Optab
{
	uchar	as;
	char	a1;
	char	a2;
	char	type;
	long	size;
	// char	param;
	char vers;
};
struct
{
	Optab*	start;
	Optab*	stop;
} oprange[ALAST];

enum
{
	STRINGSZ	= 200,
	MAXIO		= 8192,
	MAXHIST		= 20,				/* limit of path elements for history symbols */
	DATBLK		= 1024,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	AUTINC		= 0,
	AUTOFF		= 0,
	PAROFF		= 0,

	/* mark flags */
	LEAF		= 1<<0,
	FOLL		= 1<<1,
	VIS		= 1<<2,

	STEXT		= 1,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SLEAF,
	SFILE,
	SCONST,
	SPMDATA,
	SPMBSS,

	V0 =	0,
	V1 = 1,
	V2 = 2,

	C_NONE		= 0,

	C_ZREG,		/* 30 */
	C_PREG,		/* 24, 26, 28, 30 pair */
	C_HREG,		/* 16+ */
	C_REG,
	C_FREG,
	C_SREG,		/* status register */

	C_BCON,		/* 0 to 7 */
	C_ICON,		/* 0 to 31 */
	C_QCON,		/* 0 to 63 */
	C_KCON,		/* 0 to 255 */
	C_LCON,		/* other */

	C_SACON,
	C_LACON,

	C_SECON,
	C_LECON,

	C_BBRA,		/* -63 to 63 */
	C_CBRA,		/* -64 to 63 */
	C_SBRA,		/* -2048 to 2047 */
	C_LBRA,		/* rest */

	C_SAUTO,
	C_LAUTO,

	C_SEXT,
	C_LEXT,

	C_ZOZREG,	/* zero offset from Z reg */
	C_ZOPREG,	/* zero offset from X, Y, Z reg */
	C_ZOREG,
	C_SOPREG,	/* small offset from X, Y, Z reg */
	C_SOREG,
	C_LOREG,

	C_PREREG,	/* pre-increment */
	C_POSTREG,	/* post-increment */
	C_POSTZREG,	/* from Z reg */

	C_ANY,
	C_GOK,

	C_NCLASS
};

EXTERN union
{
	struct
	{
		uchar	obuf[MAXIO];			/* output buffer */
		uchar	ibuf[MAXIO];			/* input buffer */
	} u;
	char	dbuf[1];
} buf;

#define	cbuf	u.obuf
#define	xbuf	u.ibuf

EXTERN	long	HEADR;			/* length of header */
EXTERN	int	HEADTYPE;		/* type of header */
EXTERN	long	INITDAT;		/* data location */
EXTERN	long	INITRND;		/* data round above text location */
EXTERN	long	INITTEXT;		/* text location */
EXTERN	char*	INITENTRY;		/* entry point */

EXTERN	long	autosize;
EXTERN	Biobuf	bso;
EXTERN	long	bsssize;
EXTERN	int	cbc;
EXTERN	uchar*	cbp;
EXTERN	int	cout;
EXTERN	Auto*	curauto;
EXTERN	Auto*	curhist;
EXTERN	Prog*	curp;
EXTERN	Prog*	curtext;
EXTERN	Prog*	datap;
EXTERN	Prog*	prog_movsw;
EXTERN	Prog*	prog_movdw;
EXTERN	Prog*	prog_movws;
EXTERN	Prog*	prog_movwd;
EXTERN	long	datsize;
EXTERN	char	debug[128];
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi8[8];
EXTERN	Sym*	hash[NHASH];
EXTERN	Sym*	histfrog[MAXHIST];
EXTERN	int	histfrogp;
EXTERN	int	histgen;
EXTERN	char*	library[50];
EXTERN	char*	libraryobj[50];
EXTERN	int	libraryp;
EXTERN	int	xrefresolv;
EXTERN	char*	hunk;
EXTERN	char	inuxi1[1];
EXTERN	char	inuxi2[2];
EXTERN	char	inuxi4[4];
EXTERN	Prog*	lastp;
EXTERN	long	lcsize;
EXTERN	char	literal[32];
EXTERN	int	nerrors;
EXTERN	long	nhunk;
EXTERN	char*	noname;
EXTERN	long	instoffset;
EXTERN	char*	outfile;
EXTERN	long	pc;
EXTERN	long pmdatsize, pmbsssize;
EXTERN	long	symsize;
EXTERN	long	staticgen;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	tothunk;
EXTERN	char	xcmp[C_NCLASS][C_NCLASS];
EXTERN	int	version;
EXTERN	Prog	zprg;
EXTERN	int	zversion;
EXTERN	int	dtype;

extern	Optab	optab[];
extern	char*	anames[];
extern	char*	cnames[];

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	Rconv(Fmt*);
int	aclass(Adr*, Prog*);
void	addhist(long, int);
void	histtoauto(void);
void	addnop(Prog*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmlc(void);
void	asmr(void);
void	asms(int);
int	asmout(Prog*, Optab*, int);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	cflush(void);
int	cmp(int, int);
int	compound(Prog*);
void	con(Adr*, int);
Prog*	copyprg(Prog*, Prog*);
double	cputime(void);
void	datblk(long, long, int);
void	diag(char*, ...);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
int	find1(long, int);
void	follow(void);
void	gethunk(void);
void	hputl(long);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
int	isnop(Prog*);
int	isize(Prog*);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
void	initmuldiv(void);
Sym*	lookup(char*, int);
Sym*	lookup0(char*);
void	lput(long);
void	lputl(long);
void	mkfwd(void);
void*	mysbrk(ulong);
void	names(void);
Prog*	newprg(Prog*);
void	nocache(Prog*);
void	noops(void);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(void*, void*);
long	opcode(int);
Optab*	oplook(Prog*);
void	patch(void);
void	pmdata(Prog*);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, long, int);
long	regoff(Adr*, Prog*);
int	relinv(int);
int	relne(int);
int	relop(int);
int	relu(int);
long	rnd(long, long);
void	sched(Prog*, Prog*);
void	span(void);
void	undef(void);
void	xdefine(char*, int, long);
void	xfol(Prog*);
