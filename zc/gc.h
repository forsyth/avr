#include	"../cc/cc.h"
#include	"../zc/z.out.h"

/*
 * Atmel AVR
 */
#define	SZ_CHAR		1
#define	SZ_SHORT	2
#define	SZ_INT		2	// 4
#define	SZ_LONG		4
#define	SZ_IND		2	// 4
#define	SZ_FLOAT	4
#define	SZ_VLONG	8
#define	SZ_DOUBLE	8
#define	FNX		100

typedef	struct	Adr	Adr;
typedef	struct	Prog	Prog;
typedef	struct	Case	Case;
typedef	struct	C1	C1;
typedef	struct	Multab	Multab;
typedef	struct	Hintab	Hintab;
typedef	struct	Var	Var;
typedef	struct	Reg	Reg;
typedef	struct	Rgn	Rgn;
typedef	struct	Static	Static;

struct	Adr
{
	long	offset;
	double	dval;
	char	sval[NSNAME];
	Ieee	ieee;

	Sym*	sym;
	char	type;
	char	reg;
	char	name;
	char	etype;
};
#define	A	((Adr*)0)

#define	INDEXED	9
struct	Prog
{
	Adr	from;
	Adr	to;
	Prog*	link;
	long	lineno;
	uchar	as;
	char	size;
	// char	reg;
};
#define	P	((Prog*)0)

struct	Case
{
	Case*	link;
	long	val;
	long	label;
	char	def;
};
#define	C	((Case*)0)

struct	C1
{
	long	val;
	long	label;
};

struct	Multab
{
	long	val;
	char	code[20];
};

struct	Hintab
{
	ushort	val;
	char	hint[10];
};

struct	Var
{
	long	offset;
	Sym*	sym;
	char	name;
	char	etype;
};

struct	Reg
{
	long	pc;
	long	rpo;		/* reverse post ordering */

	Bits	set;
	Bits	use1;
	Bits	use2;

	Bits	refbehind;
	Bits	refahead;
	Bits	calbehind;
	Bits	calahead;
	Bits	regdiff;
	Bits	act;

	long	regu;
	long	loop;		/* could be shorter */

	
	Reg*	log5;
	long	active;

	Reg*	p1;
	Reg*	p2;
	Reg*	p2link;
	Reg*	s1;
	Reg*	s2;
	Reg*	link;
	Prog*	prog;
};
#define	R	((Reg*)0)

#define	NRGN	600
struct	Rgn
{
	Reg*	enter;
	short	cost;
	short	varno;
	short	regno;
};

#define	CSTATIC0	(CSTATIC+1)

struct	Static
{
	Sym*	sym;
	int		class;
	Static*	next;
};

EXTERN	long	breakpc;
EXTERN	Case*	cases;
EXTERN	Node	constnode;
EXTERN	Node	fconstnode;
EXTERN	long	continpc;
EXTERN	long	curarg;
EXTERN	long	cursafe;
EXTERN	Prog*	firstp;
EXTERN	Prog*	lastp;
EXTERN	long	maxargsafe;
EXTERN	int	mnstring;
EXTERN	Multab	multab[20];
EXTERN	int	retok;
EXTERN	int	hintabsize;
EXTERN	Node*	nodrat;
EXTERN	Node*	nodret;
EXTERN	Node*	nodsafe;
EXTERN	long	nrathole;
EXTERN	long	nstring;
EXTERN	Prog*	p;
EXTERN	long	pc;
EXTERN	Node	regnode;
EXTERN	char	string[NSNAME];
EXTERN	Sym*	symrathole;
EXTERN	Node	znode;
EXTERN	Prog	zprog;
EXTERN	char	reg[NREG+NFREG];
EXTERN	long	exregoffset;
EXTERN	long	exfregoffset;

#define	BLOAD(r)	band(bnot(r->refbehind), r->refahead)
#define	BSTORE(r)	band(bnot(r->calbehind), r->calahead)
#define	LOAD(r)		(~r->refbehind.b[z] & r->refahead.b[z])
#define	STORE(r)	(~r->calbehind.b[z] & r->calahead.b[z])

#define	bset(a,n)	((a).b[(n)/32]&(1L<<(n)%32))

#define	CLOAD	4
#define	CREF	5
#define	CINF	1000
#define	LOOP	3

EXTERN	Rgn	region[NRGN];
EXTERN	Rgn*	rgp;
EXTERN	int	nregion;
EXTERN	int	nvar;

EXTERN	Bits	externs;
EXTERN	Bits	params;
EXTERN	Bits	consts;
EXTERN	Bits	addrs;

EXTERN	long	regbits;
EXTERN	long	exregbits;

EXTERN	int	change;
EXTERN	int	suppress;

EXTERN	Reg*	firstr;
EXTERN	Reg*	lastr;
EXTERN	Reg	zreg;
EXTERN	Reg*	freer;
EXTERN	Var	var[NVAR];
EXTERN	long*	idom;
EXTERN	Reg**	rpo2r;
EXTERN	long	maxnr;

extern	char*	anames[];
extern	Hintab	hintab[];

/*
 * sgen.c
 */
void	codgen(Node*, Node*);
void	gen(Node*);
Static*	lookupstatic(Sym *);
void	noretval(int, int);
void	usedset(Node*, int);
void	xcom(Node*);
int	bcomplex(Node*, Node*);
void	commulinit(void);
int	commul(Node*);
int	unarith(Node*);

/*
 * cgen.c
 */
void	cgen(Node*, Node*);
void	reglcgen(Node*, Node*, Node*);
void	lcgen(Node*, Node*);
void	bcgen(Node*, int);
void	boolgen(Node*, int, Node*);
void	sugen(Node*, Node*, long);
void	layout(Node*, Node*, int, int, Node*);

/*
 * txt.c
 */
int	hireg(Node*);
void	ginit(void);
void	gclean(void);
void	nextpc(void);
void	gargs(Node*, Node*, Node*);
void	garg1(Node*, Node*, Node*, int, Node**);
Node*	nodconst(long, Node*);
Node*	nod32const(vlong, Node*);
Node*	nodfconst(double);
void	nodreg(Node*, Node*, int);
void	regret(Node*, Node*);
int	tmpreg(void);
void	regalloc(Node*, Node*, Node*, int);
void	regfree(Node*);
int	regget(int, long, int, int);
void	regialloc(Node*, Node*, Node*);
void	reginc(int, int, Node*);
void	regsalloc(Node*, Node*);
void	regaalloc1(Node*, Node*);
void	regaalloc(Node*, Node*);
void	regargfree(void);
void	regind(Node*, Node*);
void	gprep(Node*, Node*);
void	raddr(Node*, Prog*);
void	naddr(Node*, Adr*);
void	gmovm(Node*, Node*);
void	gmove(Node*, Node*);
void	gins(int a, Node*, Node*);
void	gopcode(int, Node*, Node*);
void	gopcode2(int, Node*, Node*);
int	samaddr(Node*, Node*);
void	gbranch(int);
void	patch(Prog*, long);
int	sconst(Node*);
int	sval(long);
void	gpseudo(int, Sym*, Node*);
int	retreg(int);
int	argreg(int);

/*
 * swt.c
 */
int	swcmp(const void*, const void*);
void	doswit(Node*);
void	swit1(C1*, int, long, Node*);
void	cas(void);
void	bitload(Node*, Node*, Node*, Node*, Node*);
void	bitstore(Node*, Node*, Node*, Node*, Node*);
long	outstring(char*, long);
int	mulcon(Node*, Node*, int);
Multab*	mulcon0(long);
void	nullwarn(Node*, Node*);
void	sextern(Sym*, Node*, long, long);
void	gextern(Sym*, Node*, long, long);
void	outcode(void);
void	ieeedtod(Ieee*, double);

/*
 * list
 */
void	listinit(void);
int	Pconv(Fmt*);
int	Aconv(Fmt*);
int	Dconv(Fmt*);
int	Sconv(Fmt*);
int	Nconv(Fmt*);
int	Bconv(Fmt*);
int	Rconv(Fmt*);

/*
 * reg.c
 */
Reg*	rega(void);
int	rcmp(const void*, const void*);
void	regopt(Prog*);
void	addmove(Reg*, int, int, int);
Bits	mkvar(Adr*);
void	prop(Reg*, Bits, Bits);
void	loopit(Reg*, long);
void	synch(Reg*, Bits);
ulong	allreg(ulong, Rgn*);
void	paint1(Reg*, int);
ulong	paint2(Reg*, int);
void	paint3(Reg*, int, long, int);
void	addreg(Adr*, int);

/*
 * peep.c
 */
void	peep(void);
void	excise(Reg*);
Reg*	uniqp(Reg*);
Reg*	uniqs(Reg*);
int	regtyp(Adr*);
int	regzer(Adr*);
int	anyvar(Adr*);
int	subprop(Reg*);
int	copyprop(Reg*);
void	constprop(Adr*, Adr*, Reg*);
int	copy1(Adr*, Adr*, Reg*, int);
int	copyu(Prog*, Adr*, Adr*);

int	copyas(Adr*, Adr*);
int	copyau(Adr*, Adr*);
int	copyau1(Prog*, Adr*);
int	copysub(Adr*, Adr*, Adr*, int);
int	copysub1(Prog*, Adr*, Adr*, int);

long	RtoB(int, int);
long	FtoB(int);
int	BtoR(long, int);
int	BtoF(long);

void predicate(void); 
int	isbranch(Prog *); 
int	predicable(Prog *p); 
int	modifiescpsr(Prog *p); 

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"B"	Bits
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"R"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*
