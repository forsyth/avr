/*
 * avr interpreter
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

typedef struct Instr Instr;
typedef struct Opcode Opcode;
typedef struct Func Func;

struct Opcode {
	ushort	op;
	ushort	mask;
	int	class;
	char*	mnemonic;
	void	(*f)(Opcode*, Instr*);
	char*	ken;
	int	ct;
	int	freq;
};

struct Instr {
	int	size;
	ulong	addr;	/* byte address of instruction */
	ulong	wpc;		/* word pc of instruction */
	ushort	op;
	uchar	class;
	uchar	rr;
	uchar	rd;
	uchar	rrw;	/* Rr for movw */
	uchar	rdw;	/* Rd for movw */
	uchar	ri;	/* r[16,31] in Rd position */
	uchar	rj;	/* r[16,31] in Rr position */
	uchar	rw;	/* r24, 26, 28, 30 (2 bit) */
	uchar	k8;	/* 8-bit value */
	uchar	kq;	/* literal [0, 63] */
	uchar	oq;	/* offset [0, 63] */
	uchar	b;	/* bit number [0, 7]*/
	uchar	s;	/* sreg flag number [0, 7] */
	uchar	sb;	/* sreg flag number in brb[cs] */
	uchar	ia;	/* IO register [0,63] in in/out */
	uchar	sa;	/* IO register [0,31] in skip */
	short	cbrdisp;	/* conditional branch displacement [-64, 63] (words) */
	short	brdisp;	/* unconditional branch/call displacement [-2048, 2047] (words) */
	ulong	dest;	/* long call/jump destination */
	ulong	daddr;	/* lds/sts data address [0,65535], also with RAMPD */
	ushort	w[2];	/* instruction words */
};

struct Func {
	char *s;
	int	f;
	Func *nxt;
};

enum
{
	/* these are aliased in data space at n+0x20 */
	IOeecr=	0x1c,
	IOeedr=	0x1d,
	IOeearl=	0x1e,
	IOeearh=	0x1f,
	IOsreg=	0x3f,
	IOsph=	0x3e,
	IOspl=	0x3d,

	IOgimsk=	0x1b,	/* general interrupt mask */
	IOgifr=	0x1a,	/* general interrupt function register */
	IOtimsk=	0x19,	/* timer key */
	IOtifr=	0x18,	/* timer function */

	IOudr=	0x0c,	/* uart data register */
	IOusr=	0x0b,	/* uart status register */
	IOucr=	0x0a,	/* uart control register */
	IOubr=	0x09,	/* uart baud rate */

	Nreg=	32,
	Nsio=	64,	/* standard IO */
	Nio=	Nsio+160,
	Sramsize=	4096
};

/* cycle time variants */
enum
{
	C0 = 25,
	C12,
	C123,
	C34,
	C45,
};

static	uchar	mem[Nreg+Nio+Sramsize];
#define	reg	mem	/* the registers are mapped into data space at 0 */
#define	iomem	(&mem[Nreg])	/* IO space follows the registers */

static	uchar eeprom[4096];
static	int	sreg[8];	/* bits in sreg */
static	int	SP;
static	int	skip1;
static	int	PC, NPC;
static	uchar*	XR= reg+26;
static	uchar*	YR= reg+28;
static	uchar*	ZR= reg+30;

static	jmp_buf	except;

static	int	ni;
static	int	ct;
static	int	branch;

#define	STACKSZ	64

static	Func	*fns;
static	Func	*stack[STACKSZ];
static 	int	nstack;

static	void	stats(void);

#define	OPD(v, d)	((v) | (((d)&0x1F)<<4))
#define	OPBRB(v, k, s)	((v) | (((k)&0x7F)<<3) | ((s)&7))
#define	OPRD(v, r, d)	((v) | (((r)&0x10)<<5) | (((d)&0x1F)<<4) | ((r)&0xF))

#define	OPIDB(v, i, d)	((v) | (((i)&0xF0)<<4) | (((d)&0xF)<<4) | ((i)&0xF))
#define	OPIDW(v, i, d)	((v) | (((i)&0x30)<<2) | (((d)&6)<<2) | ((i)&0xF))
#define	OPIO(v, i, d)	((v) | (((i)&0x30)<<2) | (((d)&0x1F)<<4) | ((i)&0xF))
#define	OPIOB(v, i, d)	((v) | (((d)&0x1F)<<3) | ((i)&7))

static	ushort	code[8*8192];

enum {
	C, Z, N, V, S, H, T, I
};

char* brbs[] = {
	"BLO",
	"BEQ",
	"BMI",
	"BVS",
	"BLT",
	"BHS",
	"BTS",
	"BIE"
};

char* brbc[] = {
	"BSH",
	"BNE",
	"BPL",
	"BVC",
	"BGE",
	"BHC",
	"BTC",
	"BID"
};

static	int	tflag = 1;
static	int	cflag;
static	int	dflag;
static	int	iflag;

#define	TR	if(tflag)itrace
#define	TRC	if(cflag&!tflag)itrace

static void
trap(char *f, ...)
{
	char buf[256];
	va_list ap;
	Fmt fmt;

	fmtfdinit(&fmt, 2, buf, sizeof buf);
	va_start(ap, f);
	fmtprint(&fmt, "zi: trap: ");
	fmtvprint(&fmt, f, ap);
	fmtprint(&fmt, " pc=%.4ux\n", PC);
	va_end(ap);
	fmtfdflush(&fmt);
	longjmp(except, 1);
}

static char*
sregs(void)
{
	static char s[10], sn[] = "CZNVSHTI";
	char *p;
	int i;

	p = s;
	for(i=0; i<7; i++)
		if(sreg[i])
			*p++ = sn[i];
	*p = 0;
	return s;
}

static void
itrace(char *f, ...)
{
	char buf[256];
	va_list ap;
	Fmt fmt;

	fmtfdinit(&fmt, 1, buf, sizeof buf);
	va_start(ap, f);
	fmtprint(&fmt, "%4.4ux %4.4ux\t", PC, code[PC]);
	fmtvprint(&fmt, f, ap);
	fmtprint(&fmt, "\t'%s'\n", sregs());
	va_end(ap);
	fmtfdflush(&fmt);
}

static char*
fn(int pc)
{
	Symbol s;

	if(findsym(2*pc, CTEXT, &s))
		return s.name;
	return "?";
}

static void
call(int pc)
{
	Symbol s;
	char *n;
	Func *p;

	if(findsym(2*pc, CTEXT, &s))
		n = s.name;
	else
		n = nil;
	for(p = fns; p != nil; p = p->nxt)
		if(n == p->s)
			break;
	if(p == nil){
		p = (Func*)malloc(sizeof(struct Func));
		p->s = n;
		p->f = 0;
		p->nxt = fns;
		fns = p;
	}
	if(nstack >= STACKSZ){
		fprint(2, "stack overflow\n");
		exits("stack");
	}
	stack[nstack++] = p;
}

static void
ret(void)
{
	if(nstack < 1){
		fprint(2, "stack underflow\n");
		exits("stack");
	}
	nstack--;
}

static int
cycle(int ct, int nop, int size)
{
	if(nop){
		if(size == 2)
			return 1;	/* extra one for C123 below */
		return 0;
	}
	switch(ct){
	case C0:
		return 5;	/* ? */
	case C12:
		if(branch){
			branch = 0;
			return 2;
		}
		return 1;
	case C123:
		if(skip1)
			return 2;	/* 1 added later when needed */
		return 1;
	case C34:
		return 3;	/* 16 bit PC */
	case C45:
		return 4;	/* 16 bit PC */
	default:
		return ct;
	}
}

#pragma varargck argpos itrace 1
#pragma varargck argpos trap 1

#define	UNIMP(s)	USED(i); unimp((s))

static void
unimp(char *s)
{
	itrace("UNIMP[%s]", s);
	trap("unimp: %s", s);
}

static void
setf(int v)
{
	sreg[V] = 0;
	sreg[S] = sreg[N] = (v&0x80)!=0;	/* S = N ^ V */
	sreg[Z] = v == 0;
}

static int
getw(uchar *r)
{
	return (r[1]<<8) | r[0];
}

static void
putw(uchar *r, int v)
{
	r[0] = v;
	r[1] = v>>8;
}

static int
getcb(ulong a)
{
	int b;
	ushort w;

	b = a & 1;
	a >>= 1;
	if(a >= nelem(code))
		trap("bad code address: %.4lux", a);
	w = code[a];
	if(b)
		w >>= 8;
	return w & 0xFF;
}

static void
IUpush(ulong v)
{
	if(SP < 0 || SP >= sizeof(mem))
		trap("stack range: 0x%.4ux", SP);
	mem[SP] = v;
	SP--;
}

static void
IUpush2(ulong v)
{
	IUpush(v);
	IUpush(v>>8);
}

static ulong
IUpop(void)
{
	SP++;
	if(SP < 0 || SP >= sizeof(mem))
		trap("stack range: 0x%.4ux", SP);
	return mem[SP];
}

static ulong
IUpop2(void)
{
	ulong v;

	v = IUpop();
	return (v << 8) | IUpop();
}

static void
IUout(int d, int s)
{
	int i;

	if(d < 0 || d >= Nio)
		trap("invalid IO offset: 0x%.2ux", d);
	iomem[d] = s;
	switch(d){
	case IOsreg:
		for(i=0; i<7; i++)
			sreg[i] = (iomem[d] & (1<<i)) != 0;
		break;
	case IOsph:
	case IOspl:
		SP = getw(&iomem[IOspl]);
		break;
	case IOudr:
		write(1, &iomem[IOudr], 1);
		break;
	case IOeecr:
		if((iomem[IOeecr] & (1<<2)) == 0)
			s &= ~(1<<1);	/* write enable requires master write enable */
		if(s & (1<<0)){
			iomem[IOeedr] = eeprom[getw(&iomem[IOeearl]) & (sizeof(eeprom)-1)];
			s = 0;
		}
		if(s & (1<<1)){
			eeprom[getw(&iomem[IOeearl]) & (sizeof(eeprom)-1)] = iomem[IOeedr];
			s = 0;
		}
		iomem[IOeecr] = s;
		break;
	}
}

static int
IUin(int d)
{
	int i, s;

	if(d < 0 || d >= Nio)
		trap("invalid IO offset: 0x%.2ux", d);
	switch(d){
	case IOsreg:
		s = 0;
		for(i=0; i<7; i++)
			s |= sreg[i] << i;
		iomem[d] = s;
		break;
	case IOsph:
	case IOspl:
		putw(&iomem[IOspl], SP);
		break;
	case IOudr:
		read(0, &iomem[IOudr], 1);
		break;
	case IOusr:
		iomem[IOusr] |= 1<<5;	/* data buffer empty */
		break;
	case IOeedr:
		break;
	}
	return iomem[d];
}

static uchar*
datalv(ulong a)
{
	if(a >= nelem(mem))
		trap("bad data address: %.4lux", a);
	return &mem[a];
}

static uchar
load(ulong a)
{
	if(a < 0x20)
		trap("read of register: %.4lux", a);
	if(a < 0x100)
		return IUin(a-0x20);
	return *datalv(a);
}

static void
store(ulong a, uchar v)
{
	if(a < 0x20)
		trap("write to register: %.4lux", a);
	if(a < 0x100){
		IUout(a-0x20, v);
		return;
	}
	*datalv(a)  = v;
}

static void
IUaddb(uchar *d, int r, int c)
{
	int d7, r7, R;

	d7 = *d & 0x80;
	r7 = r & 0x80;
	R = (*d + r + c) & 0xFF;
	*d = R;
	R &= 0x80;
	sreg[C] = ((d7&r7) | (r7&~R) | (d7&~R)) != 0;
	sreg[V] = ((d7&r7&~R) | (~d7&~r7&R)) != 0;
	sreg[N] = (*d & 0x80) != 0;
	sreg[S] = sreg[N] ^ sreg[V];
	sreg[Z] = *d == 0;
	/* TO DO: H */
}

static void
IUsubb(uchar *d, int r, int c)
{
	int d7, r7, R, o;

	o = *d;
	d7 = o & 0x80;
	r7 = r & 0x80;
	R = (o - r - c) & 0xFF;
	*d = R;
	R &= 0x80;
	sreg[C] = ((~d7&r7) | (r7&R) | (R&~d7)) != 0;
	sreg[V] = ((d7&~r7&~R) | (~d7&r7&R)) != 0;
	sreg[N] = (*d & 0x80) != 0;
	sreg[S] = sreg[N] ^ sreg[V];
	sreg[Z] = *d == 0;
	/* TO DO: H */
}

static void
iadc(Opcode*, Instr *i)
{
	IUaddb(&reg[i->rd], reg[i->rr], sreg[C]);
	TR("ADDBC\tR%d, R%d [=%.2ux]", i->rr, i->rd, reg[i->rd]);
}

static void
iadd(Opcode*, Instr *i)
{
	IUaddb(&reg[i->rd], reg[i->rr], 0);
	TR("ADDB\tR%d, R%d [=%.2ux]", i->rr, i->rd, reg[i->rd]);
}

static void
iadiw(Opcode*, Instr *i)
{
	IUaddb(&reg[i->rw], i->kq, 0);
	IUaddb(&reg[i->rw+1], 0, sreg[C]);
	sreg[Z] = getw(&reg[i->rw]) == 0;
	TR("ADIW\t$0x%ux,R%d [=%.2ux%.2ux]", i->kq, i->rw, reg[i->rw+1], reg[i->rw]);
}

static void
iand(Opcode*, Instr *i)
{
	reg[i->rd] &= reg[i->rr];
	setf(reg[i->ri]);
	TR("ANDB\tR%d,R%d [=%.2ux]", i->rr, i->rd, reg[i->ri]);
}

static void
iandi(Opcode*, Instr *i)
{
	reg[i->ri] &= i->k8;
	setf(reg[i->ri]);
	TR("ANDB\t$#%.2ux,R%d [=%.2ux]", i->k8, i->ri, reg[i->ri]);
}

static void
iasr(Opcode*, Instr *i)
{
	sreg[C] = reg[i->rd] & 1;
	reg[i->rd] = (reg[i->rd] & 0x80) | (reg[i->rd] >> 1);
	setf(reg[i->rd]);
	sreg[V] = sreg[N] ^ sreg[C];
	sreg[S] = sreg[N] ^ sreg[V];
}

static void
ibclr(Opcode*, Instr *i)
{
	sreg[i->s] = 0;
	TR("BCLR\t%d", i->s);
}

static void
ibld(Opcode*, Instr *i)
{
	reg[i->rd] |= sreg[T] << i->b;
	TR("BLD\t%d,R%d [=%.2ux]", i->b, i->rd, reg[i->rd]);
}

static void
ibrbc(Opcode*, Instr *i)
{
	TR("%s\t&%.4ux", brbc[i->sb], PC + i->cbrdisp + 1);
	if(sreg[i->sb] == 0){
		NPC = PC + i->cbrdisp + 1;
		branch = 1;
	}
}

static void
ibrbs(Opcode*, Instr *i)
{
	TR("%s\t&%.4ux", brbs[i->sb], PC + i->cbrdisp + 1);
	if(sreg[i->sb] != 0){
		NPC = PC + i->cbrdisp + 1;
		branch = 1;
	}
}

static void
ibreak(Opcode*, Instr*)
{
	TR("BREAK");
	fprint(2, "zi: stopped\n");
	if(iflag)
		stats();
	exits(0);
}

static void
ibset(Opcode*, Instr *i)
{
	sreg[i->s] = 1;
	TR("BSET\t%d", i->s);
}

static void
ibst(Opcode*, Instr *i)
{
	sreg[i->s] = (reg[i->rd] & (1 << i->b)) != 0;
	TR("BST\tR%d,%d", i->rd, i->s);
}

static void
icall(Opcode*, Instr *i)
{
	IUpush2(PC + 2);
	NPC = i->dest;
	TR("CALL\t&%.4ux [ %s ]", NPC, fn(NPC));
	TRC("CALL\t&%.4ux [ %s ]", NPC, fn(NPC));
	if(iflag)
		call(NPC);
}

static void
icom(Opcode*, Instr *i)
{
	reg[i->rd] = ~reg[i->rd];
	setf(reg[i->rd]);
	sreg[C] = 1;
	TR("COMB\tR%d [=%.2ux]", i->rd, reg[i->rd]);
}

static void
icbi(Opcode*, Instr *i)
{
	TR("CBI	%d,IO(0x%.2ux) [=%.2ux]", i->b, i->sa, iomem[i->sa]&~(1<<i->b));
	IUout(i->sa, iomem[i->sa] & ~(1<<i->b));
}

static void
icp(Opcode*, Instr *i)
{
	uchar t;

	t = reg[i->rd];
	IUsubb(&t, reg[i->rr], 0);
	TR("CMP\tR%d, R%d", i->rr, i->rd);
}

static void
icpc(Opcode*, Instr *i)
{
	uchar t;
	int z;

	z = sreg[Z];
	t = reg[i->rd];
	IUsubb(&t, reg[i->rr], sreg[C]);
	if(sreg[Z])
		sreg[Z] = z;
	TR("CMPC\tR%d, R%d", i->rr, i->rd);
}

static void
icpi(Opcode*, Instr *i)
{
	uchar t;

	t = reg[i->ri];
	IUsubb(&t, i->k8, 0);
	TR("CMP	$%d, R%d [0x%.2ux :: %.2ux]", i->k8, i->ri, i->k8, reg[i->ri]);
}

static void
icpse(Opcode*, Instr *i)
{
	skip1 = reg[i->rr] == reg[i->rd];
	TR("CPSE	R%d, R%d [%c]", i->rr, i->rd, skip1? 'y': 'n');
}

static void
idec(Opcode*, Instr *i)
{
	int r;

	r = reg[i->rd];
	reg[i->rd]--;
	setf(reg[i->rd]);
	sreg[V] = r == 0x80;
	sreg[S] = sreg[N] ^ sreg[V];
	TR("DEC\tR%d [=%.2ux]", i->rd, reg[i->rd]);
}

static void
ieicall(Opcode*, Instr *i)
{
	UNIMP("eicall");
}

static void
ieijmp(Opcode*, Instr *i)
{
	UNIMP("eijmp");
}

static void
ielpm(Opcode*, Instr *i)
{
	UNIMP("elpm");
}

static void
ieor(Opcode*, Instr *i)
{
	reg[i->rd] ^= reg[i->rr];
	setf(reg[i->ri]);
	TR("EORB\tR%d,R%d [=%.2ux]", i->rr, i->rd, reg[i->rd]);
}

static void
ifmul(Opcode*, Instr *i)
{
	UNIMP("fmul");
}

static void
ifmuls(Opcode*, Instr *i)
{
	UNIMP("fmuls");
}

static void
ifmulsu(Opcode*, Instr *i)
{
	UNIMP("fmulsu");
}

static void
iicall(Opcode*, Instr*)
{
	TR("CALL	(Z) [=&%4.4ux] [ %s ]", getw(ZR), fn(getw(ZR)));
	TRC("CALL	(Z) [=&%4.4ux] [ %s ]", getw(ZR), fn(getw(ZR)));
	IUpush2(PC+1);
	NPC = getw(ZR);
	if(iflag)
		call(NPC);
}

static void
iijmp(Opcode*, Instr*)
{
	TR("JMP	(Z) [=&%4.4ux]", getw(ZR));
	NPC = getw(ZR);
}

static void
iin(Opcode*, Instr *i)
{
	TR("IN	$0x%.2ux, R%d", i->ia, i->rd);
	reg[i->rd] = IUin(i->ia);
}

static void
iinc(Opcode*, Instr *i)
{
	int r;

	r = reg[i->rd];
	reg[i->rd]++;
	setf(reg[i->rd]);
	sreg[V] = r == 0x7F;
	sreg[S] = sreg[N] ^ sreg[V];
	TR("INC\tR%d [=%.2ux]", i->rd, reg[i->rd]);
}

static void
ijmp(Opcode*, Instr *i)
{
	TR("JMP\t&%.4lux", i->dest);
	NPC = i->dest;
}

static void
ild(Opcode *o, Instr *i)
{
	int q, pre, post;
	uchar *rp;
	ulong addr;
	char fmt[64];

	snprint(fmt, sizeof(fmt), "MOVB	%s [@%%4.4lux=%%.2ux]", o->ken);
	if(i->w[0] & 0x1000){
		q = 0;
		pre = (i->w[0] & 3) == 2;
		post = (i->w[0] & 3) == 1;
		switch(i->w[0] & 0xf){
		case 1: case 2:
			rp = ZR;
			break;
		case 9: case 0xA:
			rp = YR;
			break;
		case 0xC: case 0xD: case 0xE:
			rp = XR;
			break;
		default:
			trap("illegal instruction: %4.4ux", i->w[0]);
			return;
		}
	}
	else{
		q = i->oq;
		pre = 0;
		post = 0;
		rp = (i->w[0] & 0x8) ? YR : ZR;
	}
	if(pre)
		putw(rp, getw(rp)-1);
	addr = getw(rp) + q;
	reg[i->rd] = load(addr);
	if(post)
		putw(rp, getw(rp)+1);
	if(q == 0){
		TR(fmt, i->rd, addr, reg[i->rd]);
		return;
	}
	TR("MOVB	%d(%s), R%d	[@%4.4lux=%.2ux]", q, rp == YR ? "Y" : "Z", i->rd, addr, reg[i->rd]);
}

static void
ildo(Opcode *o, Instr *i)
{
	uchar *rp;
	ulong addr;
	char fmt[64];

	snprint(fmt, sizeof(fmt), "MOVB	%s [@%%4.4lux=%%2.2ux]", o->ken);
	switch(i->w[0] & 0xF){
	case 0: case 1: case 2:	/* Z */
		rp = ZR;
		break;
	case 9: case 0xA: case 0xB:	/* Y */
		rp = YR;
		break;
	case 0xC: case 0xD: case 0xE:	/* X */
		rp = XR;
		break;
	default:
		trap("illegal instruction: %4.4ux", i->w[0]);
		return;
	}
	if((i->w[0] & 3) == 2)
		putw(rp, getw(rp)-1);	/* pre-decrement */
	addr = getw(rp);
	reg[i->rd] = load(addr);
	TR(fmt, i->rd, addr, reg[i->rd]);
	if((i->w[0] & 3) == 1)
		putw(rp, addr+1);	/* post-increment */
}

static void
ildd(Opcode*, Instr *i)
{
	uchar *rp;
	ulong addr;
	int r;

	if(i->w[0] & 8){
		rp = YR;
		r = 'Y';
	}else{
		rp = ZR;
		r = 'Z';
	}
	addr = getw(rp) + i->oq;
	reg[i->rd] = load(addr);
	TR("MOVB	%d(%c), R%d [@%4.4lux=%2.2ux]", i->oq, r, i->rd, addr, reg[i->rd]);
}

static void
ildi(Opcode*, Instr *i)
{
	reg[i->ri] = i->k8;
	TR("MOVB	$%d, R%d [=%.2ux]", i->k8, i->ri, reg[i->ri]);
}

static void
ilds(Opcode*, Instr *i)
{
	TR("MOVB	&%4.4lux, R%d [=%.2ux]", i->daddr, i->rd, reg[i->rd]);
	reg[i->rd] = load(i->daddr);
}

static void
ilpm(Opcode*, Instr *i)
{
	switch(i->w[0] & 0xF){
	case 0x8:
		reg[0] = getcb(getw(ZR));
		TR("LPM	(Z), R0 [=%.2ux]", reg[0]);
		break;
	case 0x4:
		reg[i->rd] = getcb(getw(ZR));
		TR("LPM	(Z), R%d [=%.2ux]", i->rd, reg[i->rd]);
		break;
	case 0x5:
		reg[i->rd] = getcb(getw(ZR));
		putw(ZR, getw(ZR)+1);
		TR("LPM	(Z)+, R%d [=%.2ux]", i->rd, reg[i->rd]);
		break;
	default:
		UNIMP("lpm");
	}
}

static void
ilsr(Opcode*, Instr *i)
{
	sreg[C] = reg[i->rd] & 1;
	reg[i->rd] >>= 1;
	setf(reg[i->rd]);
	sreg[V] = sreg[N] ^ sreg[C];
	sreg[S] = sreg[N] ^ sreg[V];
	TR("LSR	R%d [=%4.4ux]", i->rd, reg[i->rd]);
}

static void
imov(Opcode*, Instr *i)
{
	reg[i->rd] = reg[i->rr];
	TR("MOVB\tR%d, R%d [=%.2ux]", i->rr, i->rd, reg[i->rd]);
}

static void
imovw(Opcode*, Instr *i)
{
	reg[i->rdw] = reg[i->rrw];
	reg[i->rdw+1] = reg[i->rrw+1];
	TR("MOVW\tR%d, R%d [=%2.2ux%2.2ux]", i->rrw, i->rdw, reg[i->rdw+1], reg[i->rdw]);
}

static void
imuls(Opcode*, Instr *i)
{
	int v;

	v = (schar)reg[i->ri] * (schar)reg[i->rj];
	reg[0] = v&0xff;
	reg[1] = (v>>8)&0xff;
	TR("MULS	R%d, R%d [=%.2ux%.2ux ]", i->rj, i->ri, reg[1], reg[0]);
	sreg[Z] = v == 0;
	sreg[C] = (v & 0x8000) != 0;
}

static void
imulsu(Opcode*, Instr *i)
{
	int v;

	v = (schar)reg[i->ri] * reg[i->rj];
	reg[0] = v&0xff;
	reg[1] = (v>>8)&0xff;
	TR("MULSU	R%d, R%d [=%.2ux%.2ux ]", i->rj, i->ri, reg[1], reg[0]);
	sreg[Z] = v == 0;
	sreg[C] = (v & 0x8000) != 0;
}

static void
imulu(Opcode*, Instr *i)
{
	int v;

	v = reg[i->rd] * reg[i->rr];
	reg[0] = v&0xff;
	reg[1] = (v>>8)&0xff;
	TR("MULU	R%d, R%d [=%.2ux%.2ux ]", i->rr, i->rd, reg[1], reg[0]);
	sreg[Z] = v == 0;
	sreg[C] = (v & 0x8000) != 0;
}

static void
ineg(Opcode*, Instr *i)
{
	reg[i->rd] = -reg[i->rd];
	setf(reg[i->rd]);
	sreg[C] = reg[i->rd] != 0;
	sreg[V] = reg[i->rd] == 0x80;
	sreg[S] = sreg[N] ^ sreg[V];
	TR("NEGB\tR%d [=%.2ux]", i->rd, reg[i->rd]);
}

static void
inop(Opcode*, Instr*)
{
	TR("NOP");
}

static void
ior(Opcode*, Instr *i)
{
	reg[i->rd] |= reg[i->rr];
	setf(reg[i->rd]);
	sreg[V] = 0;
	TR("ORB\tR%d,R%d [=%.2ux]", i->rr, i->rd, reg[i->ri]);
}

static void
iori(Opcode*, Instr *i)
{
	reg[i->ri] |= i->k8;
	setf(reg[i->ri]);
	sreg[V] = 0;
	TR("ORB	$0x%4.4ux, R%d [=%4.4ux]", i->k8, i->ri, reg[i->ri]);
}

static void
iout(Opcode*, Instr *i)
{
	TR("OUT\tR%d,0x%.2ux", i->rd, i->ia);
	IUout(i->ia, reg[i->rd]);
}

static void
ipop(Opcode*, Instr *i)
{
	reg[i->rd] = IUpop();
	TR("POPB\tR%d [=%.2ux]", i->rd, reg[i->rd]);
}

static void
ipush(Opcode*, Instr *i)
{
	IUpush(reg[i->rd]);
	TR("PUSHB\tR%d", i->rd);
}

static void
ircall(Opcode*, Instr *i)
{
	IUpush2(PC + 1);
	NPC = PC + i->brdisp + 1;
	TR("RCALL\t&%.4ux [%d] [ %s ]", NPC, i->brdisp + 1, fn(NPC));
	TRC("RCALL\t&%.4ux [%d] [ %s ]", NPC, i->brdisp + 1, fn(NPC));
	if(iflag)
		call(NPC);
}

static void
iret(Opcode*, Instr*)
{
	TR("RET");
	NPC = IUpop2();
	if(iflag)
		ret();
}

static void
ireti(Opcode*, Instr*)
{
	TR("RETI");
	sreg[I] = 1;
	NPC = IUpop2();
	if(iflag)
		ret();
}

static void
irjmp(Opcode*, Instr *i)
{
	NPC = PC + i->brdisp + 1;
	TR("RJMP\t%d =%4.4ux", i->brdisp, NPC);
}

static void
iror(Opcode*, Instr *i)
{
	int c;

	c = reg[i->rd] & 1;
	reg[i->rd] = (sreg[C] << 7) | (reg[i->rd] >> 1);
	setf(reg[i->rd]);
	sreg[C] = c;
	sreg[V] = sreg[N] ^ c;
	sreg[S] = sreg[N] ^ sreg[V];
	TR("RORB	R%d [=%4.4ux]", i->rd, reg[i->rd]);
}

static void
isbc(Opcode*, Instr *i)
{
	int z;

	z = sreg[Z];
	IUsubb(&reg[i->rd], reg[i->rr], sreg[C]);
	if(sreg[Z])
		sreg[Z] = z;
	TR("SUBBC\tR%d, R%d [=%.2ux]", i->rr, i->rd, reg[i->rd]);
}

static void
isbci(Opcode*, Instr *i)
{
	int z;

	z = sreg[Z];
	IUsubb(&reg[i->ri], i->k8, sreg[C]);
	if(sreg[Z])
		sreg[Z] = z;
	TR("SUBBC\t$%d [=%.2ux], R%d [=%.2ux]", i->k8, i->k8, i->ri, reg[i->ri]);
}

static void
isbi(Opcode*, Instr *i)
{
	TR("SBI	%d,IO(0x%.2ux) [=%.2ux]", i->b, i->sa, iomem[i->sa]|(1<<i->b));
	IUout(i->sa, iomem[i->sa] | (1<<i->b));
}

static void
isbic(Opcode*, Instr *i)
{
	TR("SBIC	%d,IO(0x%.2ux)", i->b, i->sa);
	skip1 = (IUin(i->sa) & (1<<i->b)) == 0;
}

static void
isbis(Opcode*, Instr *i)
{
	TR("SBIS	%d,IO(0x%.2ux)", i->b, i->sa);
	skip1 = (IUin(i->sa) & (1<<i->b)) != 0;
}

static void
isbiw(Opcode*, Instr *i)
{
	IUsubb(&reg[i->rw], i->kq, 0);
	IUsubb(&reg[i->rw+1], 0, sreg[C]);
	sreg[Z] = getw(&reg[i->rw]) == 0;
	TR("SBIW	$0x%ux,R%d [=%.2ux%.2ux]", i->kq, i->rw, reg[i->rw+1], reg[i->rw]);
}

static void
isbrc(Opcode*, Instr *i)
{
	TR("SBRC	%d, R%d [=%4.4ux]", i->b, i->rd, reg[i->rd]);
	skip1 = (reg[i->rd] & (1<<i->b)) == 0;
}

static void
isbrs(Opcode*, Instr *i)
{
	TR("SBRS	%d, R%d [=%4.4ux]", i->b, i->rd, reg[i->rd]);
	skip1 = (reg[i->rd] & (1<<i->b)) != 0;
}

static void
isleep(Opcode*, Instr*)
{
	TR("SLEEP");
	fprint(2, "zi: slept\n");
	exits(0);	/* TO DO: pending interrupts */
}

static void
ispm(Opcode*, Instr *i)
{
	UNIMP("spm");	/* store program memory (variable) */
}

static void
ist(Opcode *o, Instr *i)
{
	int q, pre, post;
	uchar *rp;
	ulong addr;
	char fmt[64];

	snprint(fmt, sizeof(fmt), "MOVB	%s [@%%4.4lux=%%.2ux]", o->ken);
	if(i->w[0] & 0x1000){
		q = 0;
		pre = (i->w[0] & 3) == 2;
		post = (i->w[0] & 3) == 1;
		switch(i->w[0] & 0xf){
		case 1: case 2:
			rp = ZR;
			break;
		case 9: case 0xA:
			rp = YR;
			break;
		case 0xC: case 0xD: case 0xE:
			rp = XR;
			break;
		default:
			trap("illegal instruction: %4.4ux", i->w[0]);
			return;
		}
	}
	else{
		q = i->oq;
		pre = 0;
		post = 0;
		rp = (i->w[0] & 0x8) ? YR : ZR;
	}
	if(pre)
		putw(rp, getw(rp)-1);
	addr = getw(rp) + q;
	store(addr, reg[i->rd]);
	if(post)
		putw(rp, getw(rp)+1);
	if(q == 0){
		TR(fmt, i->rd, addr, reg[i->rd]);
		return;
	}
	TR("MOVB	R%d, %d(%s)	[@%4.4lux=%.2ux]", i->rd, q, rp == YR ? "Y" : "Z", addr, reg[i->rd]);
}
	
static void
isto(Opcode *o, Instr *i)
{
	uchar *rp;
	ulong addr;
	char fmt[64];

	snprint(fmt, sizeof(fmt), "MOVB	%s [@%%4.4lux=%%.2ux]", o->ken);
	switch(i->w[0] & 0xF){
	case 0: case 1: case 2:	/* Z */
		rp = ZR;
		break;
	case 9: case 0xA: case 0xB:	/* Y */
		rp = YR;
		break;
	case 0xC: case 0xD: case 0xE:	/* X */
		rp = XR;
		break;
	default:
		trap("illegal instruction: %4.4ux", i->w[0]);
		return;
	}
	if((i->w[0] & 3) == 2)
		putw(rp, getw(rp)-1);	/* pre-decrement */
	addr = getw(rp);
	TR(fmt, i->rd, addr, reg[i->rd]);
	store(addr, reg[i->rd]);
	if((i->w[0] & 3) == 1)
		putw(rp, addr+1);	/* post-increment */
}

static void
istd(Opcode*, Instr *i)
{
	uchar *rp;
	ulong addr;
	int r;

	if(i->w[0] & 8){
		rp = YR;
		r = 'Y';
	}else{
		rp = ZR;
		r = 'Z';
	}
	addr = getw(rp) + i->oq;
	TR("MOVB	R%d,%d(%c) [@%4.4lux=%.2ux]", i->rd, i->oq, r, addr, reg[i->rd]);
	store(addr, reg[i->rd]);
}

static void
ists(Opcode*, Instr *i)
{
	TR("MOVB	R%d, &%4.4lux", i->rd, i->daddr);
	store(i->daddr, reg[i->rd]);
}

static void
isub(Opcode*, Instr *i)
{
	IUsubb(&reg[i->rd], reg[i->rr], 0);
	TR("SUBB\tR%d, R%d [=%.2ux]", i->rr, i->rd, reg[i->rd]);
}

static void
isubi(Opcode*, Instr *i)
{
	IUsubb(&reg[i->ri], i->k8, 0);
	TR("SUBB\t$%d [=%.2ux], R%d [=%.2ux]", i->k8, i->k8, i->ri, reg[i->ri]);
}

static void
iswap(Opcode*, Instr *i)
{
	int v;

	v = reg[i->rd];
	reg[i->rd] = ((v >> 4) & 0xF) | ((v & 0xF) << 4);
	TR("SWAP\tR%d [=%.2ux]", i->rd, reg[i->rd]);
}

static void
iwdr(Opcode*, Instr*)
{
	TR("WDR");
}

static int
get2x(int pc, ushort *w)
{
	if(pc < 0 || pc >= nelem(code)){
		werrstr("pc out of range");
		return -1;
	}
	*w = code[pc];
	return 0;
}

static int
decode(ulong pc, Instr *i)
{
	ushort w;

	if(get2x(pc, &w) < 0){
		werrstr("can't read instruction: %r");
		return -1;
	}
//fprint(2, "pc=%4.4ux %4.4ux\n", pc, w);
	i->size = 1;
	i->addr = pc;
	i->wpc = pc>>1;
	i->op = w;	/* will be masked later */
	i->w[0] = w;
	i->b = w&7;
	i->k8 = ((w>>4)&0xF0) | (w&0xF);
	i->kq = ((w>>2)&0x30) | (w&0xF);
	i->oq = ((w>>8)&040) | ((w>>7)&030) | (w&7);	/* packed in where they could ... */
	i->rd = (w>>4)&0x1F;
	i->rr = ((w>>5)&0x10) | (w&0xF);
	i->rw = 24+((w>>3)&6);
	i->s = (w>>4)&7;
	i->sb = w & 7;
	i->cbrdisp = ((w>>3)&0x7F);
	if(i->cbrdisp & 0x40)
		i->cbrdisp |= ~0 << 7;
	i->brdisp = w & 0xFFF;
	if(i->brdisp & 0x800)
		i->brdisp |= ~0 << 12;
	i->dest = (((w>>3)&0x3E) | (w & 1))<<16;	/* subsequent word or'd in by call/jmp decoding */
	i->ri = 0x10 | ((w>>4)&0xF);
	i->rj = 0x10 | (w&0xF);
	i->rdw = (w>>3) & 0x1E;
	i->rrw = (w<<1) & 0x1E;
	i->ia = ((w>>5)&0x30) | (w&0xF);
	i->sa = (w>>3) & 0x1F;
	i->daddr = 0;	/* value provided by lds/sts decoding */

	/* multi-word instructions */
	switch(i->op & 0xFE0F){
	case 0x9000:	/* LDS */
	case 0x9200:	/* STS */
		if(get2x(pc+1, &w) < 0)
			return -1;
		i->size = 2;
		i->w[1] = w;
		i->daddr = w;
		break;
	}
	switch(i->op & 0xFE0E){
	case 0x940C:	/* JMP */
	case 0x940E:	/* CALL */
		if(get2x(pc+1, &w) < 0)
			return -1;
		i->size = 2;
		i->w[1] = w;
		i->dest |= w;
		break;
	default:
		break;
	}
	return 1;
}

static	char	*sbits[] = {"C", "Z", "N", "V", "S", "H", "T", "I"};

static	char	*brsbits[2][8] = {
	{"CC", "NE", "PL", "VC", "GE", "HC", "TC", "IC"},
	{"CS", "EQ", "MI", "VS", "LT", "HS", "TS", "IS"},
};

static	Opcode*	opindex[16];

static	Opcode	opcodes[] = {
#define	CAT2(b)	b
#define	CAT(a,b)	CAT2(a)b
#define	X(fn,as,op,mask,class,fmt,ct)	{(op),(mask),(class),(as),(CAT(i,fn)),fmt,ct}
#include "zops.h"
	{0},
};

static void
opinit(void)
{
	Opcode *o;

	for(o = opcodes; o->mnemonic != nil; o++)
		if(opindex[o->op>>12] == nil)
			opindex[o->op>>12] = o;
}

static void
stats(void)
{
	/* Opcode *o1, *o2, o; */
	Func *p, *q, f;

	fprint(2, "zi: %d instructions in %d cycles\n", ni, ct);

/*
	for(o1 = opcodes; o1->mnemonic != nil; o1++){
		for(o2 = o1+1; o2->mnemonic != nil; o2++){
			if(strcmp(o1->mnemonic, o2->mnemonic) == 0){
				o1->freq += o2->freq;
				o2->freq = 0;
			}
		}
	}
	for(o1 = opcodes; o1->mnemonic != nil; o1++){
		for(o2 = o1+1; o2->mnemonic != nil; o2++){
			if(o1->freq < o2->freq){
				o = *o1;
				*o1 = *o2;
				*o2 = o;
			}
		}
	}
	for(o1 = opcodes; o1->mnemonic != nil; o1++)
		if(o1->freq != 0)
			fprint(2, "%s:	%d\n", o1->mnemonic, o1->freq);
*/

	for(p = fns; p != nil; p = p->nxt){
		for(q = p->nxt; q != nil; q = q->nxt){
			if(p->f < q->f){
				f = *p;
				*p = *q;
				p->nxt = f.nxt;
				f.nxt = q->nxt;
				*q = f;
			}
		}
	}
	for(p = fns; p != nil; p = p->nxt)
		if(p->f != 0)
			fprint(2, "%s:	%d\n", p->s, p->f);
}

static void
run1(int nop)
{
	int k;
	Instr *i, ins;
	Opcode *o;
	char buf[100];

	i = &ins;
	if(decode(PC, i) < 0)
		trap("%r");
	NPC = PC + i->size;
	for(o = opindex[i->op>>12]; o != nil && o->mnemonic != nil; o++)
		if((i->op & o->mask) == o->op){
			i->op &= o->mask;
			if(nop == 0)
				(*o->f)(o, i);
			if(iflag){
				if(nop == 0){
					ni++;
					o->freq++;
					if(nstack > 0){
						stack[nstack-1]->f++;
						if(iflag > 1){
							for(k = 0; k < nstack-1; k++)
								stack[k]->f++;
						}
					}
				}
				ct += cycle(o->ct, nop, i->size);
			}
			return;
		}
	snprint(buf, sizeof(buf), "%.4ux", i->w[0]);
	unimp(buf);
}

/*
static ulong
beswal(ulong l)
{
	union {
		uchar v[4];
		ulong	l;
	} u;
	int i;

	u.l = l;
	l = 0;
	for(i=0; i<4; i++)
		l = (l<<8) | u.v[i];
	return l;
}
*/

static void
initsyms(int fd)
{
	int h;
	Fhdr fp;

	seek(fd, 0, 0);
	h = crackhdr(fd, &fp);
	if(!h){
		fprint(2, "cannot crack header\n");
		return;
	}
	machbytype(fp.type);
	syminit(fd, &fp);
}

void
main(int argc, char **argv)
{
	int s, fd;
	Exec exec;
	ulong txtsize, datsize;
	char *f;
	uchar *dataloc;

	memset(eeprom, 0xFF, sizeof(eeprom));
	ARGBEGIN{
	case 't':
		tflag = 0;
		break;
	case 'c':
		cflag = 1;
		break;
	case 'd':
		dflag = 1;	/* load data automatically */
		break;
	case 'i':
		iflag++;
		break;
	}ARGEND
	if((f = *argv) == nil){
		f = "z.out";
		/* fprint(2, "Usage: zi z.out\n"); */
		/* exits("usage"); */
	}
	fd = open(f, OREAD);
	if(fd < 0){
		fprint(2, "zi: can't open %s: %r\n", f);
		exits("open");
	}
	if(readn(fd, &exec, sizeof(Exec)) != sizeof(Exec)){
		fprint(2, "zi: short exec header\n");
		exits("format");
	}
	txtsize = beswal(exec.text);
	datsize = beswal(exec.data);
	if(txtsize > sizeof(code)){
		fprint(2, "zi: code size (%lud) too large\n", txtsize);
		exits("size");
	}
	if(readn(fd, code, txtsize) != txtsize){
		fprint(2, "zi: can't read code: %r\n");
		exits("size");
	}
	if(datsize+beswal(exec.bss) > Sramsize){
		fprint(2, "zi: data size (%lud) too large\n", datsize+beswal(exec.bss));
		exits("size");
	}
	dataloc = (uchar*)code+((txtsize+1)&~1);	/* load after text */
	if(dflag)
		dataloc = mem+0x100;
	if(readn(fd, dataloc, datsize) != datsize){
		fprint(2, "zi: can't read data: %r\n");
		exits("size");
	}
	initsyms(fd);
	close(fd);
	opinit();
	PC = beswal(exec.entry);
	fprint(2, "entry %4.4ux\n", PC);
	if(setjmp(except))
		exits("error");
	for(;;){
		s = skip1;
		skip1 = 0;
		run1(s);
		PC = NPC;
	}
	exits(0);
}
