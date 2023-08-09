#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

static char Eflashread[] = "flashread";

/*
 * it would be faster to use the USART in synchronous mode but
 * it sends start/stop bits, which the flash won't like
 */

enum {
	/* port A */
	CSn_o=	1<<3,

	/* port D */
	Xck1_o=	1<<5,
	Tx_o=	1<<3,
	Rx_i=	1<<2,
	Rx_intr=	2,

	Trace=	0,
};

/*
 * low-level bit-banging for SPI mode 0 (idle clock low)
 */

static void
flashlowinit(void)
{
	iomem.ddrd |= Xck1_o;	/* we provide clock */
	iomem.porta |= CSn_o;	/* it's active low, so initialise it high */
	iomem.portd &= ~Xck1_o;	/* force low inactive state */
	iomem.ddra |= CSn_o;
	iomem.ddrd |= Tx_o;
	iomem.ddrd &= ~Rx_i;
	iomem.eicra |= 3<<(Rx_intr*2);	/* rising edge */
}

void
flashenable(void)
{
}

void
flashselect(byte n)
{
	byte s;

	s = splhi();
	if(n){
		iomem.portd &= ~Xck1_o;
		iomem.porta &= ~CSn_o;
	}else
		iomem.porta |= CSn_o;
	wait250ns();	/* more than enough recovery/setup time */
	splx(s);
}

void
flashdisable(void)
{
	byte s;

	s = splhi();
	iomem.portd &= ~Xck1_o;
	iomem.porta |= CSn_o;
	splx(s);
}

static void
flashputc(byte c)
{
	byte s, z, o, b;

	s = splhi();
	z = iomem.portd & ~(Xck1_o | Tx_o);
	for(b = 0x80; b != 0; b >>= 1){
		o = z;
		if(c & b)
			o |= Tx_o;	/* set now to latch on rising edge */
		iomem.portd = o;
		iomem.portd |= Xck1_o;
	}
	iomem.portd = z;
	splx(s);
}

static byte
flashgetc(void)
{
	byte s, i, z, o, b;

	s = splhi();
	z = iomem.portd & ~(Xck1_o | Tx_o);
	o = z | Xck1_o;
	i = 0;
	for(b = 0; b < 8; b++){
		iomem.portd = z;
		iomem.portd = o;
		iomem.portd = z;	/* Rx available shortly after falling edge */
		i <<= 1;
		if(iomem.pind & Rx_i)
			i |= 1;
	}
	iomem.portd = z;
	splx(s);
	return i;
}

static void
flashclock(void)
{
	byte s;

	s = splhi();
	iomem.portd &= ~Xck1_o;
	iomem.portd |= Xck1_o;
	iomem.portd ^= Xck1_o;
	splx(s);
}

/*
 * Atmel AT45DB041
 */

enum {
	Pagesize=	264,
	Datasize=	256,
	Npage2blk=	8,

	/* group A */
	MemPageRead=	0x52,
	MemBuf1Read=	0x53,
	MemBuf2Read=	0x55,
	MemBuf1Cmp=	0x60,
	MemBuf2Cmp=	0x61,
	Buf1Prog=	0x83,
	Buf2Prog=	0x86,
	Buf1WriteProg=	0x82,
	Buf2WriteProg=	0x85,
	AutoRewrite1=	0x58,
	AutoRewrite2=	0x59,
	PageErase=	0x81,
	BlockErase=	0x50,

	/* group B */
	Buf1Read=	0x54,
	Buf2Read=	0x56,
	Buf1Write=	0x84,
	Buf2Write=	0x87,
	ReadStatus=	0x57,

	Nopage=	0xFFFF,
	Nobuf=	2,
};

typedef struct Fbuf Fbuf;
struct Fbuf {
	ushort	page;	/* current page, or Nopage */
	byte	dirty;	/* waiting to go out */
	byte	read;		/* being read */
	Rendez	r;
};

static struct {
	byte	busy;
	Rendez	r;
	Fbuf	buf[2];
	byte	b;	/* next buffer to use */
} flash;

static byte bufwriteprog[] = {Buf1WriteProg, Buf2WriteProg};
static byte pagerewrite[] = {AutoRewrite1, AutoRewrite2};
static byte readbuffer[] = {Buf1Read, Buf2Read};
static byte readtobuffer[] = {MemBuf1Read, MemBuf2Read};

static byte
buffered(ushort p)
{
	if(flash.buf[0].page == p)
		return 0;
	if(flash.buf[1].page == p)
		return 1;
	return Nobuf;
}

static byte
claimbuf(void)
{
	byte b;

	b = flash.b;
	flash.b ^= 1;
	flash.buf[b].page = Nopage;
	return b;
}

static byte
iodone(void*)
{
	return !flash.busy;
}

void
flashintr(void)
{
	iomem.eimsk &= ~Rx_i;
	iomem.eifr = Rx_i;
	if(flash.busy){
		flash.busy = 0;
		wakeup(&flash.r);
		if(Trace)
			uartputc('!');
	}else
		uartputc('?');
}

static byte
flashwait(void)
{
	byte s;
	ushort n;

	/* will be 8-20 ms wait for interrupt */
	flashselect(1);
	flashputc(ReadStatus);
	flashclock();	/* clock just one bit to monitor the status bit */
	flash.busy = 1;
	s = splhi();
	iomem.eifr = Rx_i;
	iomem.eimsk |= Rx_i;
	splx(s);

	sleep(&flash.r, iodone, nil);	/* TO DO &f->r */

	for(n=0; n<1000 && (iomem.pind&Rx_i) == 0; n++)
		waitusec(100);
	flashselect(0);

	return n < 1000;
}

void
flashinit(void)
{
	flashlowinit();
	flash.buf[0].page = Nopage;
	flash.buf[1].page = Nopage;
	flash.b = 0;
}

void
flashread(ushort p, ushort o, void *a, int n)
{
	byte *b;
	ushort i;
	byte x, op, nobuf;

	if(n <= 0)
		return;
	if(o+n > Pagesize)
		panic(Eflashread);
	nobuf = (p & Unbuffered) != 0;
	p &= ~Unbuffered;
	x = buffered(p);
	if(x != Nobuf){
		if(Trace)
			print("flbr %d %d %d <- %d\n", p, o, n, x);
		op = readbuffer[x];
	}else{
		if(!nobuf){
			/* read page to buffer, then read buffer */
			x = claimbuf();
			if(Trace)
				print("flrb %d %d %d -> %d\n", p, o, n, x);
			flashselect(1);
			flashputc(readtobuffer[x]);
			flashputc(p>>7);	/* 0000 pppp pppp pppb bbbb bbbb */
			flashputc((p<<1)|0);
			flashputc(0);
			flashselect(0);
			if(!flashwait())
				panic(Eflashread);	/* TO DO */
			op = readbuffer[x];
		}else{
			if(Trace)
				print("flr %d %d %d\n", p, o, n);
			op = MemPageRead;
		}
	}
	flashselect(1);
	flashputc(op);
	flashputc(p>>7);	/* 0000 pppp pppp pppb bbbb bbbb */
	flashputc((p<<1)|(o>>8));
	flashputc(o);
	flashputc(0);	/* 8 don't care (for buffer read) */
	if(op == MemPageRead){
		/* 24 more don't care */
		flashputc(0);
		flashputc(0);
		flashputc(0);
	}
	b = a;
	i = 0;
	do{
		*b++ = flashgetc();
	}while(++i < n);
	flashselect(0);

	if(op != MemPageRead)
		flash.buf[x].page = p;
}

byte
flashpagewrite(ushort p, void *a, int n, byte *m)
{
	ushort i;
	byte *b;
	byte x;

	if(Trace)
		print("flw %d\n", p);
	x = buffered(p);
	if(x == Nobuf)
		x = claimbuf();
	flash.buf[x].page = Nopage;	/* invalidate now */
	flashselect(1);
	flashputc(bufwriteprog[x]);
	flashputc(p>>7);	/* 0000 pppp pppp pppb bbbb bbbb */
	flashputc(p<<1);
	flashputc(0);
	b = a;
	for(i=0; i<n; i++)
		flashputc(*b++);
	for(; i<Datasize; i++)
		flashputc(0xFF);
	if(m != nil)
		for(; i<Pagesize; i++)
			flashputc(*m++);
	flashselect(0);

	if(!flashwait())
		return 0;
	flash.buf[x].page = p;	/* buffer valid */
	return 1;
}

byte
flashpagerewrite(ushort p)
{
	byte x;

	if(Trace)
		print("flrw %d\n", p);
	x = buffered(p);
	if(x == Nobuf)
		x = claimbuf();
	flash.buf[x].page = Nopage;	/* invalidate now */
	flashselect(1);
	flashputc(pagerewrite[x]);
	flashputc(p>>7);	/* 0000 pppp pppp pppb bbbb bbbb */
	flashputc(p<<1);
	flashputc(0);
	flashselect(0);

	if(!flashwait())
		return 0;
	flash.buf[x].page = p;	/* buffer valid */
	return 1;
}

#ifdef TESTING
void
flashtest(void)
{
	byte i;
	ushort j;
	byte *buf;

	buf = malloc(Pagesize);
	flashenable();
	for(j=0; j<nelem(buf); j++)
		buf[j] = j;
	flashpagewrite(0, buf, Pagesize, nil);
	for(i=0; i<1; i++){
		flashread(i, 0, buf, sizeof(buf));
		for(j=0; j<nelem(buf); j++){
			print(" %.2ux", buf[j]);
			if(((j+1)&7) == 0)
				print("\n");
		}
		print("\n");
	}
	flashread(0, 10, buf, 20);
	for(j=0; j<20; j++)
		print(" %.2ux", buf[j]);
	print("\n");
	flashdisable();
	free(buf);
}
#endif

