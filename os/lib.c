#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

void
moteid(void)
{
	byte i, n;

	n = ds2401readid(myid, sizeof(myid));
	print("ID:");
	if(n){
		ledtoggle(Green);
		for(i=0; i<n; i++)
			print(" %.2ux", myid[i]);
		print("\n");
		ledtoggle(Green);
	}else{
		diag(Dnoid);
		print("none\n");
	}
}

ushort
shortid(void)
{
	return (myid[1]<<8) | myid[7];
}

ushort
get2(byte *p)
{
	return (p[1]<<8) | p[0];
}

void
put2(byte *p, ushort n)
{
	p[0] = n;
	p[1] = n>>8;
}

void
putx(byte c)
{
	byte h;

	h = c;
	h >>= 4;
	uartputc(h>=10? 'A'+h-10: '0'+h);
	h = c & 0xF;
	uartputc(h>=10? 'A'+h-10: '0'+h);
}

void
puti(ushort n)
{
	putx(n>>8); putx(n&0xFF);
}

void
dump(void *a, int n)
{
	uchar *p, *e;
	int j;

	p = a;
	n = (n+15)&~0xF;
	e = p+n;
	while(p != e){
		for(j=0; j<16; j++)
			putx(*p++);
		uartputc('\n');
	}
}

void
strayintr(void)
{
	uartputs("stray\n",-1);
	diag(Dstray);
	// for(;;){}
}

/* TBS 200 bytes used on panic strings - use integer code eventually */
void
panic(char *f, ...)
{
	ledset((~Red)&7);
	splhi();
	uartputs("panic:", -1);
	uartputs(f, -1);
	uartputc('\n');
	stackdump();
	diag(Dpanic);
	diagput(f);
#ifdef RELEASE
	watchdogset(1000);
#endif
	for(;;){}
}

void
zerodiv(void)
{
	panic("/0");
}

void
ilock(Lock *l)
{
	l->spl = splhi();
	if(l->lk)
		panic("ilock");
	l->lk = 1;
}

void
iunlock(Lock *l)
{
	if(l->lk == 0)
		panic("iunlock");
	l->lk = 0;
	splx(l->spl);
}

void
stackdump(void)
{
	ushort *s;
	byte i;

	if(1)
		return;
	uartputc('\n');
	s = getsp();
	for(i=0; i<HWSTACK/BY2WD; i++){
		putx((ushort)s); uartputc('='); putx(*s++); uartputc('\n');	/* don't use print, to save stack */
	}
}

ushort
crc16block(byte *p, short n)
{
	ushort crc;

	crc = 0;
	while(--n >= 0)
		crc = crc16byte(crc, *p++);
	return crc;
}

void
uartputi(int v)
{
	uartputc((v>>8)&0xff);
	uartputc(v&0xff);
}

void
uartputl(long v)
{
	uartputi((v>>16)&0xffff);
	uartputi(v&0xffff);
}

void
eepromput2(ushort a, uint v)
{
	eepromwrite(a, (v>>8)&0xff);
	eepromwrite(a+1, v&0xff);
}

uint
eepromget2(ushort a)
{
	uint v;

	v = 0;
	v |= eepromread(a)<<8;
	v |= eepromread(a+1);
	return v;
}

void
eepromput4(ushort a, ulong v)
{
	eepromput2(a, (v>>16)&0xffff);
	eepromput2(a+2, v&0xffff);

}

ulong
eepromget4(ushort a)
{
	ulong v;

	v = 0;
	v |= (ulong)eepromget2(a)<<16;
	v |= eepromget2(a+2);
	return v;
}

enum{
	Errlen = 8,
	Diagadr = EEPROMSIZE-2*Dzed-Errlen,
};

static int diagbuf[Dzed];

void
diag(int d)
{
	diagbuf[d]++;
}

void
diagv(int d, int v)
{
	diagbuf[d] = v;
}

void
diagclr(void)
{
	ushort a;

	for(a = Diagadr; a < EEPROMSIZE; a += 2)
		eepromput2(a, 0);
}

void
diagput(char *s)
{
	int i;
	ushort a;

#ifdef RELEASE
	a = Diagadr;
	for(i = 0; i < Dzed; i++, a += 2)
		eepromput2(a, diagbuf[i]);
	if(s != nil)
		for(i = 0; i < Errlen; i++)
			eepromwrite(a++, *s == 0 ? 0 : *s++);
#endif
}

void
diaginit(void)
{
	int i;
	ushort a, v;
	char *s;

	a = Diagadr;
	for(i = 0; i < Dzed; i++, a += 2){
		v = eepromget2(a);
		print("%ud ", v);
		diagbuf[i] = v;
	}
	s = malloc(Errlen);
	for(i = 0; i < Errlen; i++)
		s[i] = eepromread(a++);
	s[Errlen-1] = 0;
	print("%s\n", s);
	free(s);
}

/*
void
diagsend(void)
{
	Block *b;

	b = allocb(sizeof(diagbuf)+2);
	if(b != nil){
		memmove(b->wp, diagbuf, sizeof(diagbuf));
		b->wp += sizeof(diagbuf);
		put2(b->wp, seconds());
		b->wp += 2;
		macwrite(sinkroute(), b);
	}
}
*/