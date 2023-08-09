#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "slip.h"

enum {
	DOSLIP=	0,	/* enable SLIP */

	/* ucsra */
	Rxc=	1<<7,	/* receive complete */
	Txc=	1<<6,	/* transmit complete (shift register empty) */
	Udre=	1<<5,	/* transmitter buffer empty */
	Fe=	1<<4,	/* frame error */
	Dor=	1<<3,	/* data overrun */
	Upe=	1<<2,	/* parity error */
	U2X=	1<<1,	/* double transmission speed */
	Mpcm=	1<<0,	/* multi-processor communication mode */

	/* ucsrb */
	Rxcie=	1<<7,	/* receiver interrupt enable */
	Txcie=	1<<6,	/* transmitter interrupt enable */
	Udrie=	1<<5,	/* transmitter buffer empty enable */
	Rxen=	1<<4,	/* enable receiver */
	Txen=	1<<3,	/* enable transmitter */
	Ucsz9a=	1<<2,	/* character size bit */
	Rxb8=	1<<1,	/* 9th data bit */
	Txb8=	1<<0,	/* 9th data bit */

	/* ucsrc */
	Umselsync=	1<<6,	/* select synchronous mode */
	Upmeven=	2<<4,	/* even parity */
	Upmodd=	3<<4,	/* odd parity */
	Usbs2=	1<<5,	/* two stop bits */
	Ucsz8=	3<<1,	/* 8-bit characters */
	UcpolLow=	1<<0,	/* transmit on falling edge, receive on rising edge */
};

typedef struct Uart Uart;

struct Uart {
	Block*	ib;	/* block coming in */
	Queue*	rq;
	Rendez	rr;

	Block*	ob;	/* block going out */
	Queue*	wq;
	Rendez	wr;

	byte	overrun;
	byte	framing;
	byte	slipping;
	byte	reading;
	ushort	slipin;	/* slip input state */
	ushort	slipout;	/* slip output state */
};

static Uart uart;
static void	uartcanread(Queue*);
static void uartcanwrite(Queue*);
void	uart0udre(void);

static	Qproto	rproto = {.hiwat=2, .canget=uartcanread};
static	Qproto	wproto = {.hiwat=2, .canput=uartcanwrite};

void
uartinit(void)
{
	uart.rq = qalloc(&rproto);
	uart.wq = qalloc(&wproto);

	/* TO DO: baud rate */
	if(DOSLIP){
		uart.ib = allocb(Blocksize);
		uart.slipping = 1;
		iomem.ucsr0b |= Rxen|Txen|Rxcie;
	}else{
		uart.rq->state |= Qcoalesce;
		iomem.ucsr0b |= Txen;
		iomem.ucsr0b &= ~(Rxen|Rxcie);
	}
}

void
uartreadenable(byte b)
{
	byte x;

	x = splhi();
	if(b)
		iomem.ucsr0b |= Rxen;
	else
		iomem.ucsr0b &= ~Rxen;
	uart.reading = b;
	splx(x);
	if(!b)
		qflush(uart.rq);
}

byte
uartctl(char *s)
{
	switch(*s){
	case 'b':
		/* TO DO: baud rate */
		return 0;
	case 's':
		/* TO DO: enable slip */
		return 0;
	default:
		return Enoctl;
	}
}

Block*
uartread(byte wait)
{
	Block *b;

	while((b = qget(uart.rq)) == nil && wait)
		sleep(&uart.rr, qcanget, uart.rq);
	return b;
}

static void
uartcanread(Queue*)
{
	wakeup(&uart.rr);
}

void
uart0rxintr(void)
{
	byte c;
	Block *b;
	Queue *q;
	ushort v;


	c = iomem.ucsr0a;
	if(c & Fe)
		uart.framing++;
	if(c & Dor)
		uart.overrun++;
	c = iomem.udr0;
	if(!uart.reading)
		return;
	if(uart.slipping){
		uart.slipin = v = slipdec(c, uart.slipin);
		b = uart.ib;
		if(b == nil){
			if(v != Slipend)
				uart.overrun++;
			return;
		}
		if(v == Slipend){
			if(b->wp == b->rp)
				return;	/* ignore empty frame */
			uart.ib = allocb(Blocksize);
			if(uart.ib == nil){
				b->wp = b->rp;	/* overrun: re-use block */
				uart.ib = b;
				uart.overrun++;
				return;
			}
			qput(uart.rq, b);
		}else if(v != Slipesc){
			if(b->wp < b->lim)
				*b->wp++ = c;
		}
		return;
	}
	if((q = uart.rq) == nil || q->state & Qflow){
		uart.overrun++;
		return;
	}
	if(q->first != nil){
		b = q->last;
		if(b->wp < b->lim){
			*b->wp++ = c;
			return;
		}
	}
	b = allocb(Blocksize);
	if(b == nil){
		uart.overrun++;
		return;
	}
	*b->wp++ = c;
	qput(q, b);
}

void
uartwrite(Block *b)
{
	byte s, c;
	ushort v;

	if(b == nil || b->wp == b->rp){
		freeb(b);
		return;
	}
	s = splhi();
	if(uart.ob == nil){
		uart.ob = b;
		if(uart.slipping){
			uart.slipout = v = slipenc(b, uart.slipout);
			c = v & 0xFF;
		}else
			c = *b->rp++;
		iomem.udr0 = c;
		iomem.ucsr0b |= Udrie;
		splx(s);
	}else{
		splx(s);
		if(!qcanput(uart.wq))
			sleep(&uart.wr, qcanput, uart.wq);
		qput(uart.wq, b);
		s = splhi();
		if(uart.ob == nil)
			uart0udre();
		splx(s);
	}
}

static void
uartcanwrite(Queue*)
{
	wakeup(&uart.wr);
}

/*
 * transmit interrupts use UDRE, not TXC
 */
void
uart0udre(void)
{
	Block *b;
	ushort v;

	b = uart.ob;
	for(;;){
		if(b){
			if(uart.slipping){
				uart.slipout = v = slipenc(b, uart.slipout);
				iomem.udr0 = v & 0xFF;
				iomem.ucsr0b |= Udrie;
				if(b->rp == b->wp){
					uart.ob = nil;
					freeb(b);
				}
				return;
			}
			if(b->rp < b->wp){
				iomem.udr0 = *b->rp++;
				iomem.ucsr0b |= Udrie;
				return;
			}
			freeb(b);
		}
		uart.ob = b = qget(uart.wq);
		if(b == nil)
			break;
	}
	iomem.ucsr0b &= ~Udrie;
}

void
uartputs(char *s, int n)
{
	byte x;

	if(n < 0)
		n = strlen(s);
	if(DOSLIP){
		x = splhi();
		uartputc(0300);
		splx(x);
	}
	while(--n >= 0 && *s){
		x = splhi();
		uartputc(*s++);
		splx(x);
	}
	if(DOSLIP){
		x = splhi();
		uartputc(0300);
		splx(x);
	}
}
