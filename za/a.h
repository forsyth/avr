
typedef	struct	Sym	Sym;
typedef	struct	Gen	Gen;
typedef	struct	Io	Io;
typedef	struct	Hist	Hist;

#define	MAXALIGN	7
#define	FPCHIP		1
#define	NSYMB		8192
#define	BUFSIZ		8192
#define	HISTSZ		20
#define	NINCLUDE	10
#define	NHUNK		10000
#define	EOF		(-1)
#define	IGN		(-2)
#define	GETC()		((--fi.c < 0)? filbuf(): *fi.p++ & 0xff)
#define	NHASH		503
#define	STRINGSZ	200
#define	NMACRO		10

#define	ALLOC(lhs, type)\
	while(nhunk < sizeof(type))\
		gethunk();\
	lhs = (type*)hunk;\
	nhunk -= sizeof(type);\
	hunk += sizeof(type);

#define	ALLOCN(lhs, len, n)\
	if(lhs+len != hunk || nhunk < n) {\
		while(nhunk <= len)\
			gethunk();\
		memmove(hunk, lhs, len);\
		lhs = hunk;\
		hunk += len;\
		nhunk -= len;\
	}\
	hunk += n;\
	nhunk -= n;

struct	Sym
{
	Sym*	link;
	char*	macro;
	long	value;
	ushort	type;
	char	*name;
	char	sym;
};
#define	S	((Sym*)0)

struct
{
	char*	p;
	int	c;
} fi;

struct	Io
{
	Io*	link;
	char	b[BUFSIZ];
	char*	p;
	short	c;
	short	f;
};
#define	I	((Io*)0)

struct
{
	Sym*	sym;
	short	type;
} h[NSYM];

struct	Gen
{
	Sym*	sym;
	long	offset;
	short	type;
	short	reg;
	short	xreg;
	short	name;
	ushort	mask;
	double	dval;
	char	sval[8];
};

struct	Hist
{
	Hist*	link;
	char*	name;
	long	line;
	long	offset;
};
#define	H	((Hist*)0)

enum
{
	CLAST,
	CMACARG,
	CMACRO,
	CPREPROC,
};

char	debug[256];
Sym*	hash[NHASH];
char*	Dlist[30];
int	nDlist;
Hist*	ehist;
int	newflag;
Hist*	hist;
char*	hunk;
char*	include[NINCLUDE];
Io*	iofree;
Io*	ionext;
Io*	iostack;
long	lineno;
int	nerrors;
long	nhunk;
int	nosched;
int	ninclude;
Gen	nullgen;
char*	outfile;
int	pass;
char*	pathname;
long	pc;
int	peekc;
int	sym;
char	symb[NSYMB];
int	thechar;
char*	thestring;
long	thunk;
Biobuf	obuf;

void	errorexit(void);
void	pushio(void);
void	newio(void);
void	newfile(char*, int);
Sym*	slookup(char*);
Sym*	lookup(void);
void	syminit(Sym*);
long	yylex(void);
int	getc(void);
int	getnsc(void);
void	unget(int);
int	escchar(int);
void	cinit(void);
void	pinit(char*);
void	cclean(void);
void	outcode(int, Gen*, int, Gen*);
void	outgcode(int, Gen*, int, Gen*, Gen*);
void	zname(char*, int, int);
void	zaddr(Gen*, int);
void	ieeedtod(Ieee*, double);
int	filbuf(void);
Sym*	getsym(void);
void	domacro(void);
void	macund(void);
void	macdef(void);
void	macexpand(Sym*, char*);
void	macinc(void);
void	macprag(void);
void	maclin(void);
void	macif(int);
void	macend(void);
void	dodefine(char*);
void	prfile(long);
void	outhist(void);
void	linehist(char*, int);
void	gethunk(void);
void	yyerror(char*, ...);
int	yyparse(void);
void	setinclude(char*);
int	assemble(char*);
int	xyzreg(int);

/*
 *	system-dependent stuff from ../cc/compat.c
 */
enum				/* keep in synch with ../cc/cc.h */
{
	Plan9	= 1<<0,
	Unix	= 1<<1,
	Windows	= 1<<2,
};
int	mywait(int*);
int	mycreat(char*, int);
int	systemtype(int);
int	pathchar(void);
char*	mygetwd(char*, int);
int	myexec(char*, char*[]);
int	mydup(int, int);
int	myfork(void);
int	mypipe(int*);
void*	mysbrk(ulong);
