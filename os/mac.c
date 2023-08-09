#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

/*
 * MAC level, based on T-MAC with a nod to S-MAC
 */

/*
 * packet format
 *	preamble[18]
 *	sync bytes[2]
 * ucb format:
 *	addr[2] type[1] group[1] length[1]
 * ucsc format:
 *	length[1] mac[8]
 * both:
 *	data[length]
 *	crc[2]
 * phys. layer records 16-bit strength and 32-bit time stamp (fine resolution)
 * and 32-bit coarse S-MAC system time (resolution of 1 ms)
 */

#define	DPRINT	if(0)print
#define	SRPRINT	if(0)print

/*
 * MAC header in data[] above
 *	len[1]
 *	type[1]
 *	dst[2]
 *	src[2]
 *	duration[2]
 *	seq[2] - omitted for now
 */
enum {
	Olen=	0,
	Otype=	Olen+1,
	Odst=	Otype+1,
	Osrc=	Odst+2,
	Odur=	Osrc+2,
	/* Oseq=	Odur+2, */
	MAChdrlen=	Odur+2,	/* len[1] type[1] dst[2] src[2] duration[2] seq[2] */

	/* SYNC */
	Oframe=	Odur,

	MACsynclen=	MAChdrlen+4,	/* len[1] type[1] dst[2] src[2] frame[2] seq[2] routing[4] */
	MACctllen=	MAChdrlen,

	Trts=	1,
	Tcts=	2,
	Tdata=	3,
	Tack=	4,
	Tsync=	5,

	Broadcast = 0xFFFF,
};

typedef struct Sched Sched;
typedef struct Node Node;

struct Sched {
	byte	nnode;
	byte	ttl;
	ushort	timer;	/* ticks to frame start */
	ushort	ilp;	/* initial listening period in a frame */
	byte	cycle;
};

struct Node {
	ushort	addr;
	byte	sn;	/* its current schedule number */
	byte	ttl;	/* frames to presumed absence */
};

/*
 * parameters
 */
enum {
	Bandwidth = 38,	/* data rate in Kbps */
	EncodingRatio = 2,	/* manchester encoding ratio (byte in gives 2 bytes out) */
	Leadersize = (18+2)*EncodingRatio,	/* preamable + SOF[2] */

	IFSspace=	5,	/* short interframe space (802.11): minimum ms to wait before sending CTS/ACK */
	RtsCW=	(1<<4),	/* number of slots in RTS contention window */
	Slottime=	2,	/* milliseconds in contention slot */
	Crcsize=	2*EncodingRatio,

	Maxattempt=	4,

	Maxmate=	16,	/* limit to number of neighbours noted */
	Maxsched=	Maxmate/2,	/* limit to number of schedules */

	Nosn = 255,
};

/* time taken to transmit a packet of len bytes */
#define	PKTTIME(len)	((Leadersize + (len)*EncodingRatio + 1 + Crcsize)*8 / Bandwidth + 1)

/* packet durations and protocol intervals */
enum {
	D_ctl=	PKTTIME(MACctllen),
	D_sync=	PKTTIME(MACsynclen),
	D_process=	2,	/* processing delay */

	I_frame=	1024,	/* nominal frame time (ms); power of 2 that divides CLOCKFREQ evenly */
	MT= 20*I_frame/100,	/* time difference within which schedules are considered the same */
	TC=	Slottime*RtsCW,
	TR=	D_ctl,	/* RTS length */
	TT=	D_process,
	TA=	15*(TC+TR+TT)/10,	/* inactivity timer (ms) */
	P_sync=	(1<<4),	/* period of our SYNC requests (frames) */

	TTL=	3*P_sync,	/* frames to presumed absence - mates */
	STTL= 2*P_sync,	/* frames to presumed absence - schedules */

	ILP= 10,	/* minimum initial listening period */
};

static struct {
	ushort	addr;	/* our address */
	short	timer;
	short	secs;
	Rendez	r;
	Rendez	busy;
	Rendez	rxr;

	byte	inframe;
	byte	timeout;
	byte	ours;
	byte	inidle;	/* mac in idle slot */
	byte	asleep;	/* mac in sleep period */
	byte	sink;	/* are we the sink */
	byte	sync;

	ushort	nav;		/* network allocation vector: time until medium free */
	ulong	clock;

	Sched	sched[Maxsched];	/* schedule 0 is ours */
	byte	nsched;

	Node	mates[Maxmate];	/* immediate neighbours */
	byte	nmate;

	Queue*	rq;
	Block*	oq;
	Block*	oqt;

	ushort	ilp;	/* initial listening period */
	ushort	idle;	/* mac idle time interval needed */

	void	(*synced)(ushort);
} mac;

static void macproc(void*);
static int rxsync(Block*);
static void mactimerinit(void);

byte
ismate(ushort a)
{
	byte i;

	if(a == Broadcast)
		return 1;
	for(i=0; i<mac.nmate; i++)
		if(mac.mates[i].addr == a)
			return 1;
	return 0;
}

static void
heardmate(ushort a)
{
	byte i, x;
	Node *m;

	for(i = 0; i < mac.nmate; i++){
		m = &mac.mates[i];
		if(m->addr == a){
			x = splhi();
			if(i < mac.nmate && m->addr == a){
				m->ttl = TTL;
				if(m->sn != Nosn)
					mac.sched[m->sn].ttl = STTL;
			}
			splx(x);
			return;
		}
	}
}

/*
static byte
anymates(byte sn)
{
	byte i;
	Node *m;

	for(i=0; i<mac.nmate; i++){
		m = &mac.mates[i];
		if(m->ttl != 0 && m->sn == sn)
			return 1;
	}
	return 0;
}
*/

static void
schednum(int osn, int sn)
{
	byte i;
	Node *m;

	for(i = 0; i < mac.nmate; i++){
		m = &mac.mates[i];
		if(m->sn == osn)
			m->sn = sn;
	}
}

static void
heardsync(ushort src, byte sn)
{
	byte i, f, x;
	Node *m;

	f = 0xFF;
	for(i=0; i<mac.nmate; i++){
		m = &mac.mates[i];
		if(m->addr == src){
			x = splhi();
			if(i < mac.nmate && m->addr == src){
				m->ttl = TTL;
				if(sn != m->sn){
					if(m->sn != Nosn)
						mac.sched[m->sn].nnode--;
					mac.sched[sn].nnode++;
				}
				m->sn = sn;
				splx(x);
				return;
			}
			splx(x);
		}
		if(m->ttl == 0)
			f = i;
	}
	if(f == 0xFF){
		f = mac.nmate;
		if(f >= Maxmate)
			return;
		x = splhi();
		f = mac.nmate++;
		mac.mates[f].ttl = TTL;
		splx(x);
	}
	m = &mac.mates[f];
	m->addr = src;
	m->ttl = TTL;
	m->sn = sn;
	mac.sched[sn].nnode++;
	//print("new mate %ud\n", src);
}

static void
newsched(byte n, ushort t)
{
	byte x;
	Sched *s;

	if(t == 0)
		t = 1;
	s = &mac.sched[n];
	s->nnode = 0;
	s->ttl = STTL;
	s->timer = t;
	s->ilp = MT/2;
	s->cycle = -1;
	if(n >= mac.nsched){
		x = splhi();
		mac.nsched = n+1;
		splx(x);
	}
}

static Qproto proto = {.hiwat=3};

void
macinit(ushort addr, void (*f)(ushort))
{
	mac.addr = addr;
	mac.asleep = 1;
	radiosleep();
	mac.rq = qalloc(&proto);
	mac.synced = f;
	mactimerinit();
	mac.sink = issink();
	mac.idle = I_frame+1;
	spawn(macproc, nil);
}

void
setmactimer(ushort n)
{
	byte s;

	s = splhi();
	mac.timeout = 0;
	mac.timer = n;
	splx(s);
}

byte
maccanwrite(void)
{
	return mac.oq == nil || mac.oq->next == nil || mac.oq->next->next == nil;
}

void
macwrite(ushort dest, Block *b)
{
	ushort duration;
	int l;

//	duration = mac.hisnav - mac.datatime;	/* mac.hisnav ``tracks time left for Tx'' */
	b = bprefix(b, MAChdrlen);
	l = BLEN(b);
	duration = PKTTIME(l);
	b->rp[Olen] = l;
	b->rp[Otype] = Tdata;
	b->rp[Odst] = dest;
	b->rp[Odst+1] = dest>>8;
	b->rp[Osrc] = mac.addr;
	b->rp[Osrc+1] = mac.addr>>8;
	b->rp[Odur] = duration;
	b->rp[Odur+1] = duration>>8;
	// b->rp[Oseq] = 0;
	// b->rp[Oseq+1] = 0;
	if(mac.oq == nil)
		mac.oq = b;
	else
		mac.oqt->next = b;
	mac.oqt = b;
}

Block*
macread(ushort *src, ushort *dst)
{
	Block *b;

	b = qget(mac.rq);
	if(b == nil)
		return nil;
	*src = get2(b->rp+Osrc);
	*dst = get2(b->rp+Odst);
	b->rp += MAChdrlen;
	return b;
}

static byte
sendctl(byte type, ushort dest, ushort duration)
{
	Block *b;

	b = allocb(Blocksize);	/* Tsync needs sink data at end */
	if(b == nil){
		DPRINT("sendctl: no mem\n");
		return 0;
	}
	b->wp[Olen] = MAChdrlen;
	b->wp[Otype] = type;
	b->wp[Odst] = dest;
	b->wp[Odst+1] = dest>>8;
	b->wp[Osrc] = mac.addr;
	b->wp[Osrc+1] = mac.addr>>8;
	b->wp[Odur] = duration;
	b->wp[Odur+1] = duration>>8;
	// b->wp[Oseq] = seq;
	// b->wp[Oseq+1] = seq>>8;
	b->wp += MAChdrlen;
	if(type == Tsync){
		addsink(b, mac.addr);
		b->rp[Olen] = BLEN(b);
	}
	if(!radiosend(b)){
		freeb(b);
		return 0;
	}
	freeb(b);
if(type==Tsync) { SRPRINT("S%d.%ld(%d)", type, mac.clock, duration); }else { SRPRINT("S%d.%ld", type, mac.clock); }
	return 1;
}

static void
sendsync(void)
{
	ushort n;
	byte x;

	if(mac.nsched == 0)
		return;
	x = splhi();
	n = mac.sched[0].timer;
	splx(x);
	// print("tx%ud:  %ld %d\n", mac.addr, mac.clock, n);
	if(sendctl(Tsync, Broadcast, n))
		DPRINT("*****%ud.%ld: SEND SYN[%d]\n", mac.addr, mac.clock, n);
	else
		DPRINT("%ud.%ld SEND SYN FAILED\n", mac.addr, mac.clock);
}

static short
framediff(short a, short b)
{
	if(a < b)
		return (I_frame+a)-b;
	return a-b;
}

static int
rxsync(Block *b)
{
	int t, t1, mt, off;
	byte x;
	ushort src, c;
	short delta, ot, xt;
	Sched *s, *ms;
	byte i, empty, sn;

	// print("rx%ud: %ld %d\n", get2(&b->rp[Osrc]), mac.clock, get2(&b->rp[Oframe]));
	x = splhi();
	off = fastticks;
	splx(x);
	off -= b->ticks;
	if(off < 0)
		off = -off;

	delta = get2(&b->rp[Oframe])-off-D_sync;
	src = get2(&b->rp[Osrc]);
	if(src == mac.addr)
		return 0;
	if(delta > I_frame)
		return 0;
	if(delta == I_frame || delta < 0)	/* can be negative after off applied when frame just about to start */
		delta = 0;
	if(mac.synced != nil)
		mac.synced(src);
	// tag = get2(&b->rp[Oseq]);
	b->rp += MAChdrlen;
	mt = I_frame;
	ms = nil;
	empty = 0;
	sn = 0;
	for(i=0; i<mac.nsched; i++){
		s = &mac.sched[i];
		if(s->ttl == 0){
			if(empty == 0)
				empty = i;
			continue;
		}
		x = splhi();
		ot = s->timer;
		splx(x);
		/* don't know whether the clocks are actually related or which might be slower */
		t = framediff(ot, delta);
		t1 = framediff(delta, ot);
		if(t1 < t)
			t = t1;	/* take the least difference */
		if(t < mt){
			mt = t;
			ms = s;
			sn = i;
		}
	}
	if((s = ms) != nil && mt >= 0 && mt <= MT){
		/* apply 50% rule */
		if(mt != 0){
			x = splhi();
			xt = ot = s->timer;
			c = 0;
			if(ot < delta){
				if(delta-ot <= I_frame/2){
					ot += mt/2;
					c = mt/2;
				}
				else
					ot -= mt/2;
			}
			else{
				if(ot-delta <= I_frame/2)
					ot -= mt/2;
				else{
					ot += mt/2;
					c = mt/2;
				}
			}
			if(ot < 0)
				ot += I_frame;
			else if(ot >= I_frame)
				ot -= I_frame;
			if(ot == 0)
				ot = 1;
			s->timer = ot;
			s->ilp = (s->ilp+c)/2;
			splx(x);
//print("*****sched %d: %d %d %d %d %d %ud\n", sn, xt, ot, delta, off+D_sync, s->ilp, src);
		}
		else{
			s->ilp /= 2;
//print("*****same sched %d: %d %d %d %d %ud\n", sn, s->timer, delta, off+D_sync, s->ilp, src);
		}
		if(s->ilp < ILP)
			s->ilp = ILP;
		s->ttl = STTL;
		heardsync(src, sn);
		return 1;
	}
	if(empty == 0){	/* no free slots */
		if(mac.nsched == Maxsched)
			return 0;
		empty = mac.nsched;
	}
	newsched(empty, delta);
//print("*****new sched %d: %d %d %ud\n", empty, delta, off+D_sync, src);
	heardsync(src, empty);
	return 1;
}

/*
 * activation events:
 *	- frame timer
 *	- receive any data
 *	- RSSI
 *	- end of transmission of data or ack
 *	- mac.nav -> 0
 */

#define	NAV(d) if((d) <= mac.nav){} else mac.nav = (d)

static Block* waitcts(Block*, ushort);
static byte	transmit(Block*);
static void	defer(Block*);

static byte
timesup(void*)
{
	return mac.timeout || mac.inframe;
}

static void
macproc(void*)
{
	Block *b, *rb;
	byte cycle, o, whole, x;
	char xmit;
	ushort dst, src, odst, dt, sink;
	int tx, ntx, rx;

	diaginit();	/* may sleep */

	if(0){
		radioidle();
		for(;;){
			radiorssi();
			// waitusec(1000);
		}
	}
	else if(0){
		radioidle();
		rx = 0;
		for(;;){
			b = radiorecv(&mac.r);
			if(b != nil){
				freeb(b);
				rx++;
				ledtoggle(Green);
				if((rx&31) == 0)
					print("%d	%ld\n", rx, seconds());
			}
		}
	}
	else if(0){
		radioidle();
		tx = ntx = 0;
		for(;;){
			if(sendctl(Tsync, Broadcast, 0)){
				tx++;
				ledtoggle(Green);
			}
			else
				ntx++;
			if(((tx+ntx)&31) == 0)
				print("%d	%ld	%d\n", tx, seconds(), ntx);
			nextsecond();
		}
	}

start:
	mac.asleep = 0;
	radioidle();
	setmactimer(15*I_frame);
	while(mac.nsched == 0 && (b = radiorecv(&mac.r)) != nil){
		if(b->rp[Otype] == Tsync){
			if(rxsync(b))
				newsink(b);
		}
		freeb(b);
	}
	setmactimer(0);
	if(mac.nsched == 0)
		newsched(0, I_frame);
	mac.sched[0].nnode++;	/* me */
	whole = 0;

	for(cycle = 0;; cycle++){
		if(mac.nmate == 0){
			if(rand15()&1){
				x = splhi();
				mac.nsched = 1;
				splx(x);
				sendsync();
			}
			else{
				x = splhi();
				mac.nsched = 0;
				splx(x);
				goto start;
			}
		}
		while(mac.nsched > 1 && mac.sched[0].nnode == 1){
			x = splhi();
			if(mac.sched[--mac.nsched].ttl){
				mac.sched[0] = mac.sched[mac.nsched];
				mac.sched[0].nnode++;	/* me */
				schednum(mac.nsched, 0);
			}
			splx(x);
			//print("adopt sched %d\n", mac.nsched);
		}
		setmactimer(0);
		mac.ours = 0;	/* race */
		mac.asleep = 1;
		radiosleep();
// print("S%d.%ld ", 0, mac.clock);
		sleep(&mac.r, timesup, nil);
// print("W%d.%ld\n", 0, mac.clock);
		mac.asleep = 0;
		radioidle();
		setmactimer(0);
		mac.inframe = 0;

		if((cycle & (P_sync-1)) == 0)
			whole++;	/* TO DO: listen for whole frame less often than this */
		
		xmit = -1;

		/* frame: (contend; send) || listen[TA] */
		for(;;){
			rb = nil;
			if(xmit < 0 && mac.ilp != 0)	/* initial listening period */
				goto Listen;
			xmit = 1;
			if(mac.nav == 0 && (mac.sync || mac.ours && mac.oq != nil && mac.sched[0].timer >= TC+TA)){
				/* (contend; send) */
				setmactimer(TC-(rand15()&(RtsCW-1))*Slottime);
				o = radiosense(&mac.r);
				setmactimer(0);
				switch(o){
				case 0:
					/* clear, transmit */
					if(mac.sync){
						sendsync();
						NAV(D_sync+D_process);
						mac.sync = 0;
						continue;
					}
					if(!mac.ours || (b = mac.oq) == nil || mac.sched[0].timer < 2*TA)
						break;
					odst = get2(b->rp+Odst);
					if(!ismate(odst)){
						// DPRINT("lost %.4ux\n", odst);
						mac.oq = b->next;
						freeb(b);
						diag(Dlostmate);
						continue;
					}
					if(odst == Broadcast){
						/* no RTS/CTS for broadcasts: fling it out (could do same for short packets) */
						if(radiosend(b)){
SRPRINT("S%d.%ld", b->rp[Otype], mac.clock);
							mac.oq = b->next;
							freeb(b);
							/* diag(Dsent); */
						}
						continue;
					}
					rb = waitcts(b, odst);
					if(rb == b){	/* cleared to send b */
						if(transmit(b)){
							mac.oq = b->next;
							freeb(b);
							/* diag(Dsent); */
							continue;
						}
						rb = nil;
					}
					defer(b);
					break;
				case 1:
					/* carrier: retry; perhaps shift by delta for retry */
					continue;
				case 2:
					/* receiving, wait up to TA for packet */
					break;
				}
			}

		Listen:
			/* listen: (SYNC? || (RTS? CTS! DATA? ACK!) || DATA?[bc]) */
			if(rb == nil){
				if(whole > 2){
					if(xmit < 0){
						xmit = 0;
						mac.ilp = 0;
					}
					setmactimer(I_frame);
					whole = 0;
				}
				else if(xmit < 0){
					xmit = 0;
					setmactimer(mac.ilp);
					mac.ilp = 0;
				}
				else
					setmactimer(mac.nav+TA);
				rb = radiorecv(&mac.r);
				setmactimer(0);
				if(rb == nil){
					if(!xmit)
						continue;
					break;
				}
SRPRINT("R%d.%ld", rb->rp[Otype], mac.clock);
			}
			src = get2(rb->rp+Osrc);
			dst = get2(rb->rp+Odst);
			dt = get2(rb->rp+Odur);
			switch(rb->rp[Otype]){
			case Tsync:
				if(rxsync(rb))
					newsink(rb);
				break;
			case Tdata:
				if(dst == Broadcast || dst == mac.addr){
					if(!qcanput(mac.rq))
						break;
					if(dst != Broadcast){
						sendctl(Tack, src, D_ctl+D_process);
						NAV(D_ctl+D_process);
						if(!mac.sink && (sink = sinkroute()) != Noroute && maccanwrite()){
							/* pass on */
							rb->rp += MAChdrlen;
							//print("%ud: route id=%ud t=%d to=%ud\n", dst, get2(rb->rp), get2(rb->rp+2), sink);
							macwrite(sink, rb);
							continue;
						}
					}
					if(mac.sink){
						qput(mac.rq, rb);
						wakeup(&mac.rxr);
						continue;
					}
				}
				break;
			case Trts:
				if(dst == mac.addr){
					/* don't reply if we haven't got room; might be more efficient to send Trnr */
					/* might also want to include indication that we've got data to send back */
					if(!qcanput(mac.rq))
						break;
					freeb(rb);
					sendctl(Tcts, src, dt);
					NAV(D_ctl+D_process+dt);
					rb = nil;
					heardmate(src);
					goto Listen;	/* strictly speaking, expect only data in this state */
				}
				/* FALL THROUGH */
			case Tcts:
				NAV(dt);
				break;
			case Tack:
				mac.nav = 0;
				break;
			}
			freeb(rb);
		}
	}
}

static Block*
waitcts(Block *b, ushort odst)
{
	Block *rb;
	byte nretry;
	ushort dst, dt;

	dt = PKTTIME(BLEN(b));
	/*
	 * RTS! CTS? DATA! ACK?
	 * currently we do this for each packet, but could do a DATA group.
	 */
	for(nretry = 0; nretry < 3; nretry++){
		sendctl(Trts, odst, dt);
		setmactimer(D_ctl+D_process+D_ctl+D_process);
		rb = radiorecv(&mac.r);
		setmactimer(0);
		if(rb == nil)
			continue;
SRPRINT("R%d.%ld", rb->rp[Otype], mac.clock);
		dst = get2(rb->rp+Odst);
		switch(rb->rp[Otype]){
		case Tcts:
			if(dst != mac.addr)
				return rb;	/* someone else got CTS => listen */
			freeb(rb);
			heardmate(odst);
			return b;
		case Trts:
			if(dst == mac.addr && get2(rb->rp+Osrc) < mac.addr)
				return rb;	/* call collision: yield to other mote */
			break;
		case Tsync:
			if(rxsync(rb))
				newsink(rb);
			break;
		case Tdata:
			if(dst == mac.addr || dst == Broadcast)
				return rb;	/* might as well accept it */
			break;
		}
		freeb(rb);
	}
	return nil;
}

static byte
transmit(Block *b)
{
	Block *rb;
	byte o;
	ushort dst, src, dt, odst;

	dt = PKTTIME(BLEN(b));
	/* finally send it */
	if(!radiosend(b))
		return 0;
SRPRINT("S%d.%ld", Tdata, mac.clock);
	setmactimer(dt+D_process+D_ctl+D_process);
	odst = get2(b->rp+Odst);
	while((rb = radiorecv(&mac.r)) != nil){
SRPRINT("R%d.%ld", rb->rp[Otype], mac.clock);
		setmactimer(0);
		dst = get2(rb->rp+Odst);
		src = get2(rb->rp+Osrc);
		o = rb->rp[Otype];
		freeb(rb);
		if(o == Tack && dst == mac.addr && src == odst)
			return 1;
	}
	setmactimer(0);
	return 0;
}

static void
defer(Block *b)
{
	if(++b->ticks > Maxattempt){
		mac.oq = b->next;
		freeb(b);
		diag(Ddeferred);
	} else if(b->next != nil){	/* push to end of queue */
		mac.oq = b->next;
		b->next = nil;
		mac.oqt->next = b;
		mac.oqt = b;
	}
}

/*
 * MAC 1ms timer
 */

enum {
	Ocie1a=	1<<4,
	Wgm12=	1<<3,	/* CTC mode, top OCR1A */
	
};

static void
mactimerinit(void)
{
	byte s;

	s = splhi();
	iomem.timsk &= ~Ocie1a;
	iomem.tcnt1h = 0;
	iomem.tcnt1l = 0;
	iomem.ocr1ah = (CLOCKFREQ/I_frame)>>8;
	iomem.ocr1al = (CLOCKFREQ/I_frame);
	iomem.tccr1b = Wgm12 | 1;
	iomem.timsk |= Ocie1a;
	splx(s);
}

void
mactimeroff(void)
{
	byte s;

	s = splhi();
	iomem.timsk &= ~Ocie1a;
	splx(s);
}

void
mactimeron(void)
{
	byte s;

	s = splhi();
	iomem.timsk |= Ocie1a;
	splx(s);
}

void
mactimer(void)
{
	byte w, i;
	ushort mt;
	Sched *s;
	Node *n;

	fastticks++;
	mac.clock++;
	if(mac.nav != 0)
		mac.nav--;

	if(--mac.secs <= 0){
		mac.secs = 1024;
		for(i=0; i<mac.nmate;){
			n = &mac.mates[i];
			if(n->ttl){
				n->ttl--;
				i++;
			}else{
				//print("%ud.%ld: lost mate %ud\n", mac.addr, mac.clock, n->addr);
				if(n->sn != Nosn)
					mac.sched[n->sn].nnode--;
				noroutemate(n->addr);	/* tell router */
				*n = mac.mates[--mac.nmate];
			}
		}
	}

	w = 0;
	if(mac.timer && --mac.timer == 0){
		mac.timeout = 1;
		w = 1;
	}

	mt = I_frame+1;

	/* schedule ticks */
	if(mac.nsched != 0){
		s = &mac.sched[0];
		if(s->timer){
			mt = s->timer;
			if(--s->timer == 0){
				mac.ours = 1;
				mac.inframe = 1;
				mac.ilp = s->ilp;
				w = 1;
				s->timer = I_frame;
				if((++s->cycle&(P_sync-1)) == 0)
					mac.sync = 1;
			}
		}
		for(i=1; i<mac.nsched; i++){
			s++;
			if(s->timer && s->ttl){
				if(s->timer < mt)
					mt = s->timer;
				if(--s->timer == 0){
					mac.inframe = 1;
					mac.ilp = s->ilp;
					w = 1;
					s->timer = I_frame;
					s->ttl--;
					if(s->ttl == 0){
						//print("%ud.%ld: lost sched %ud\n", mac.addr, mac.clock, i);
						schednum(i, Nosn);
					}
					if((++s->cycle&(P_sync-1)) == 0)
						mac.sync = 1;
				}
			}
		}
	}
	if(w)
		wakeup(&mac.r);
	if(mac.asleep && mt < I_frame+1 && mt > mac.idle){
		mac.inidle = 1;
		wakeup(&mac.busy);
	}
}

static byte
busyover(void*)
{
	return mac.inidle && mac.asleep;
}

void
macwaitslot(ushort ms)
{
	byte s;

	s = splhi();
	mac.idle = ms;
	splx(s);
	sleep(&mac.busy, busyover, nil);
	mac.idle = I_frame+1;
	mac.inidle = 0;
}

static byte
rqnotempty(void*)
{
	return qcanget(mac.rq);
}

void
macwaitrx(void)
{
	sleep(&mac.rxr, rqnotempty, nil);
}
