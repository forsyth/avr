#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "adc.h"

#define DPRINT	if(0)print

typedef struct Radio Radio;

struct Radio {
	Lock;
	byte	state;
	byte	nextb;
	Block*	rx;	/* TO DO: double buffer */
	Block*	tx;
	byte*	op;
	/* Queue*	rq; */
	ushort	crc;

	Block*	q;
	Block*	qt;
	Rendez	r;
	Rendez*	rp;
	byte	nq;

	Rendez	tr;

	/* byte	extflag; */
	byte	ntx;
	byte nrx;
	byte	len;
	byte	invert;
	byte	rxstate;
	byte	preamble;
	byte	err;
	ushort	w;	/* shift register */
	byte	shift;
	/* ushort	addr; */
	ushort	ticks;

	byte	rssidone;
	byte	sensed;
	short	carrtime;
	ushort	rssi;
	/* ushort	squelch; */

	int	send;
	int	recv;
	int	error;
};

static Radio radio;
static Adc *adcrssi;

enum {
	Sleep,
	Idle,
	Rx,
	Rss,
	Tx,
	Txd,
	Txe,
};

enum {
	/* preamble[18] sof0 sof1 data ... */
	Preamble = 0xaa,
	Preamblesize=	18,
	Sof0=	0x33,
	Sof1=	0xcc,
	Sofsize=	2,
	Crcsize=	2,
	Validprecursor=	5,
	Hdrlen=	1,	/* len[1] */
	Hdrinclen=	1,	/* header includes length */
	Pktmax=	Blocksize-2,

	/* carrier sense parameters (from S-MAC) */
	CarrierOff=	0xF5,	/* anything below it is on */
	CarrierWeak=	0x120,	/* test a few extra bytes if below this */
	Nextbytes=	3,

	/* carrier sense values */
	Cidle=	0,
	Cbusy=	1,
	Cweak=	2,
};

#define PKTSIZE(b)	(Preamblesize+Sofsize+BLEN(b)+Crcsize)

enum {
	/* Rx state */
	Rpre,
	Rsof0,
	Rsof1,
	Rhdr
};

byte spioutbyte;

#define	spiputnext(c)	(spioutbyte = (c))
// #define	spiputnext(c)	(radio.nextb = (c))

// static	byte	cbuf[32];
// static	byte	ncbuf;

static void
idlestate(void)
{
	if(radio.state == Idle)
		return;
	radio.preamble = 0;
	radio.w = 0;
	radio.shift = 0;
	radio.carrtime = 0;
	if(radio.state == Rx && radio.rxstate < Rsof0){
		radio.state = Idle;
		return;
	}
	if(radio.state == Sleep)
		cc1kwake();
	else
		spioff();
	cc1kbias(1);
	cc1krx();
	radio.state = Idle;
}

void
radioinit(void)
{
	adcrssi = adcalloc(ADC0, Scale64);
	radio.state = Sleep;
	radio.invert = 1;
	spislave();
	idlestate();
}

static void
rxreset(void)
{
	radio.rxstate = Rpre;
	radio.preamble = 0;
	radio.err = 0;
	radio.nrx = 0;
	radio.shift = 0;
	radio.crc = 0;
	radio.ticks = fastticks;
}

static byte
rxready(void*)
{
	return radio.q != nil;
}

Block*
radiorecv(Rendez *rp)
{
	Block *b;
	byte s;

	if(rp == nil)
		rp = &radio.r;
	s = splhi();
	radio.rp = rp;
	splx(s);
	if(radio.rx == nil){
		b = allocb(Blocksize);
		if(b == nil)
			DPRINT("radiorecv: no mem\n");
		s = splhi();
		if(radio.rx == nil){	/* otherwise lost race */
			radio.rx = b;
			rxreset();
			b = nil;
		}
		splx(s);
		if(b != nil)
			freeb(b);
	}
	spirx();
	sleep(rp, rxready, nil);
	ilock(&radio);
	b = radio.q;
	if(b != nil){
		radio.q = b->next;
		radio.nq--;
	}
	iunlock(&radio);
	return b;
}

static byte
etherbusy(void*)
{
	if(radio.state == Rx)
		return 2;
	/* "ether" in the original sense */
	if(radio.state != Idle || radio.carrtime == 0 && radio.sensed == Cbusy)
		return 1;
	if(radio.q != nil)
		return 2;
	return 0;
}

byte
radiosense(Rendez *rp)
{
	byte s;

	if(rp == nil)
		rp = &radio.r;
	s = splhi();
	radio.rp = rp;
	splx(s);
	/* TO DO: start RSSI sense */
	spirx();
	sleep(rp, etherbusy, nil);
	return etherbusy(nil);
}

static byte
txdone(void*)
{
	return radio.state < Rss;
}

/*
 * preamble size should be function of power?
 * tinyos uses 28, 94, 250, 371, 490, 1212, 2654 for various power modes.
 * for reception, they watch for at least 6 (high power) or 3 (others) AA/55.
 *
 * squelch (RSSI) is 0xAE (high power) or 0xEF for others.
 */
byte
radiosend(Block *b)
{
	byte s, i, ms;

	if(b == nil)
		return 1;
	s = splhi();
	if(radio.state != Idle && radio.state != Sleep){
		splx(s);
if(radio.state != Rx) DPRINT("radio.state=%d\n", radio.state);
		return 0;
	}
	if(radio.tx != nil)
		panic("radiosend");
	/* TO DO */
	/* TO DO: set up RSSI */
	/* receive mode for given period */
	/* sleep for that period on timer & rxready */
	/* if sensed || recvd, return */
	/* switch to Tx and go */

#ifdef YYY
	/* TO DO: sample RSSI for existing transmission */
	if(radio.state != Sleep){
		radio.state = Rss;
		/* back off? */
	}
#endif
	radio.tx = b;
	radio.op = b->rp;
	radio.len = BLEN(b);
	spipause();
	if(radio.state == Sleep)
		cc1kwake();
	cc1ktx();
	spitx();
	radio.state = Tx;
	spiputnext(Preamble);
	radio.ntx = 1;
	radio.preamble = Preamblesize;
	radio.crc = 0;
	mactimeroff();	/* must transmit in a bit time */
	spiputc(Preamble);
	splx(s);
	sleep(&radio.tr, txdone, nil);
	mactimeron();
	ms = PKTSIZE(b)/4+7;	/* about 1/4 ms per byte plus overhead */
	s = splhi();
	for(i = 0; i < ms; i++)
		mactimer();	/* compensate */
	splx(s);
	radio.send++;
	return 1;
}

byte
radiobusy(void)
{
	byte s;

	s = splhi();
	if(radio.state == Idle || radio.state == Sleep){
		/* request carrier sense */
		/* TO DO: must read the ADC, producing delay; if ADC is busy, claim we're busy */
		/* channels? */
		splx(s);
		return 0;
	}
	splx(s);
	return 1;
}

void
radiosleep(void)
{
	byte s;

	s = splhi();
	iomem.porta |= Green;	/* clear LED */
	if(radio.state != Sleep){
		spioff();
		cc1ksleep();
		radio.state = Sleep;
	}
	splx(s);
}

void
radioidle(void)
{
	byte s;
	static int ticks;

	s = splhi();
	iomem.porta &= ~Green;	/* set LED */
	splx(s);
	idlestate();
	if(0 && (++ticks&0xf) == 0)
		DPRINT("S=%d R=%d E=%d\n", radio.send, radio.recv, radio.error);
}

static void samplerssi(ushort);

byte
startsense(ushort nbits)
{
	byte s;

	if(!adcacquire(adcrssi, 0))
		return 0;
	s = splhi();
	radio.rssidone = 0;
	radio.sensed = Cidle;
	radio.carrtime = nbits/8;
	radio.rssi = 0x180;
	splx(s);
	return 1;
}

void
stopsense(void)
{
	radio.carrtime = 0;
	adcrelease(adcrssi);
}

static void
rssiv(ushort v)
{
	radio.rssi = v;
	if(1 || v < CarrierOff)
		radio.sensed = 1;
	adcdisable();
	adcrelease(adcrssi);
}

void
radiorssi(void)
{
	if(radio.sensed){
		// uartputi(0xfedc);
		// uartputi(radio.rssi);
		/* PdBm = -50*Vrssi-45.5 where Vrssi = 1.2*adc/1024 */
		print("%d ", -((60*radio.rssi+512)/1024+45));
		radio.sensed = 0;
	}
	/* adc < 245 on, adc < 288 ??, otherwise off */
	if(!adcacquire(adcrssi, 0))
		return;
	adcenable(adcrssi);
	adctrigger(adcrssi, rssiv);
}

static void
samplerssi(ushort v)
{
	ushort avg;

	/* scheme from S-MAC implementation */
	if(radio.state != Idle || radio.carrtime == 0)
		return;
	avg = (radio.rssi + v)/2;	/* average last two samples */
	if(avg < CarrierOff){
		radio.sensed = Cbusy;
		radio.carrtime = 0;
		/* TO DO: notify result */
	}else{
		radio.rssi = v;
		if(--radio.carrtime == 0){
			if(radio.sensed != Cweak && v < CarrierWeak){
				/* TO DO: test a few more bytes */
				radio.carrtime = Nextbytes;
				radio.sensed = Cweak;
			}else{
				radio.sensed = Cidle;
				/* TO DO: notify result */
			}
		}
	}
}

/*
 * look for byte c in word w and return its offset (0 to 7)
 * or 8 if not found.
 */
static byte
scanfor(ushort w, byte c)
{
	byte i;

	if((byte)w == c)
		return 0;
	for(i=1; i<8; i++){
		w >>= 1;
		if((byte)w == c)
			break;
	}
	return i;
}

static Block*
radiorx(byte c)
{
	byte i;
	ushort w;
	Block *b;

	w = radio.w;
	w <<= 8;
	w |= c;
	radio.w = w;
	if(radio.shift)
		c = w >> radio.shift;
	if(radio.rxstate >= Rhdr){
		if(++radio.nrx == Hdrlen){
			if(Hdrinclen)
				i = c;
			else
				i = c+Hdrlen;
			if(i > Pktmax){
				i = Pktmax;	/* could discard packet since it's clearly wrong */
				radio.err = 1;
			}
			radio.len = i;
		}
		b = radio.rx;
		if(b == nil)
			panic("radiorx");
		if(b->wp < b->lim)
			*b->wp++ = c;
		if(radio.nrx <= Hdrlen || radio.nrx <= radio.len)
			radio.crc = crc16byte(radio.crc, c);
		else if(radio.nrx >= radio.len+2){
			radio.recv++;
			/* check CRC */
			b->wp -= 2;	/* trim crc */
			if(b->wp[0] != (byte)radio.crc ||
			   b->wp[1] != (byte)(radio.crc>>8)){
				radio.err = 1;
				radio.error++;
				b->flag |= Bbadcrc;
			}
			b->ticks = radio.ticks;
			c = radio.err;
			rxreset();
			radio.state = Idle;
			if(c){
// uartputc('?');
				if(0){
					/* pass it on but with Berror set */
					b->flag |= Berror;
				}else{
// print("e=%d\n", radio.error);
					if(0){
						int j, d;

						d = 0;
						b->wp += 2;
						c = BLEN(b);
						for(i = 0; i < c; i++){
							w = b->rp[i];
							j = (w>>4)&0xf;
							uartputc(j >= 0 && j <= 9 ? j+'0' : j-10+'a');
							j = w&0xf;
							uartputc(j >= 0 && j <= 9 ? j+'0' : j-10+'a');
							if(i > 0 && b->rp[i-1] == b->rp[i])
								d = 1;
						}
						if(d)
							uartputc('D');
						uartputc('\n');
					}
					/* discard it */
					radio.rx = b;
					b->wp = b->rp;
					return nil;
				}
			}
			radio.rx = nil;
			return b;
		}
		return nil;
	}
	/* hunt the wumpus */
	if(c == Preamble || c == (Preamble>>1)){
		if(radio.state != Idle || !cc1kgetlock()){
			if(radio.rxstate > Rsof0)
				rxreset();
			if(++radio.preamble > 10){
				if(radio.carrtime != 0){
					radio.carrtime = 0;
					radio.sensed = Cbusy;
				}
				radio.state = Rx;	/* might have been Idle */
				radio.rxstate = Rsof0;
				radio.nrx = 0;
			}
		}
		if(radio.carrtime != 0)
			adctrigger(adcrssi, samplerssi);
		return nil;
	}
	if(radio.state == Idle && radio.carrtime != 0)
		adctrigger(adcrssi, samplerssi);
	switch(radio.rxstate){
	case Rpre:
		/* c isn't preamble: allow a few errors (though it bodes ill for the message) */
		if(++radio.err > 4){
			radio.preamble = 0;
			radio.err = 0;
		}
		break;
	case Rsof0:
		i = scanfor(w, Sof0);
		if(i != 8){
			radio.shift = i;
			radio.rxstate = Rsof1;
		}else	if(++radio.nrx > 3)
			rxreset();	/* start-of-frame must appear within first few bytes */
		break;
	case Rsof1:
		radio.err = 0;
		if(c == Sof1){
			radio.nrx = 0;
			radio.rxstate = Rhdr;
			radio.ticks = fastticks;
		}else
			rxreset();	/* back to the start */
		break;
	}
	return nil;
}

void
radiointr(byte c)
{
	Block *b;
	// byte c;

/*
	c = spigetc();
	spiputc(radio.nextb);
*/

	switch(radio.state){
	case Sleep:
		/* shouldn't happen */
		break;
	case Tx:
		c = radio.ntx++;
		if(c < radio.preamble)
			spiputnext(Preamble);
		else if(c < radio.preamble+1)
			spiputnext(Sof0);
		else{
			spiputnext(Sof1);
			radio.ntx = 0;
			radio.state = Txd;
		}
		break;
	case Txd:
		b = radio.tx;
		if(b == nil){
			c = radio.ntx;
			if(c == 0)
				spiputnext((byte)radio.crc);
			else if(c == 1)
				spiputnext((byte)(radio.crc>>8));
			else
				radio.state = Txe;	/* one more character time to clear CC1k */
			radio.ntx++;
			break;
		}
		c = *radio.op;
		spiputnext(c);
		radio.crc = crc16byte(radio.crc, c);
		radio.ntx++;
		if(++radio.op >= b->wp){
			radio.tx = nil;	/* state still Txd; will then send CRC above */
			radio.ntx = 0;
		}
		break;
	case Txe:
		cc1krx();
		spirx();
		rxreset();
		radio.state = Idle;
		wakeup(&radio.tr);
		/* FALL THROUGH */
	case Idle:
		if(radio.rx == nil)
			break;
		/* FALL THROUGH */
	case Rx:
		if(radio.invert)
			c = ~c;
		b = radiorx(c);
		if(b != nil){
			if(radio.nq > 3){	/* discard: probably better just to double-buffer */
// uartputc('-');
				radio.rx = b;
				b->wp = b->rp;
				break;
			}
			radio.nq++;
			b->next = nil;
			if(radio.q != nil)
				radio.qt->next = b;
			else
				radio.q = b;
			radio.qt = b;
			radio.rx = allocb(Blocksize);
			if(radio.rx == nil)
				DPRINT("radiointr: no mem\n");
			if(radio.rp != nil)
				wakeup(radio.rp);
		}
		break;
	}
}
