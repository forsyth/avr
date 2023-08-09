typedef struct Alt Alt;
typedef struct Block Block;
typedef struct Channel Channel;
typedef struct CQ CQ;
typedef struct File File;
typedef struct Label Label;
typedef struct Lock Lock;
typedef struct Proc Proc;
typedef struct QLock QLock;
typedef struct Queue Queue;
typedef struct Qproto Qproto;
typedef struct Rendez Rendez;
typedef struct Task Task;
typedef struct Timer Timer;
typedef struct Ureg Ureg;

enum {
	Blocksize = 24	/* was 48 */
};

#define	nelem(x)	(sizeof(x)/sizeof(x[0]))
#define	offsetof(s, m)	(uint)(&(((s*)0)->m))

struct Label {
	int	sp;
	int	fp;
	int	pc;
};

struct Lock {
	byte	spl;
	byte	lk;
};

struct QLock {
	byte	lk;
	Proc*	head;
	Proc*	tail;
};

struct Rendez {
	Proc*	p;
};

struct Block {
	byte*	rp;
	byte*	wp;
	byte*	lim;
	Block*	next;
	byte	flag;
	byte	ctl;
	byte	base[Blocksize];
	ushort	ticks;
	ushort	magic;	/* debugging */
};
#define BLEN(s)	((s)->wp - (s)->rp)

enum {
	/* flags */
	Bbadcrc=	1<<0,	/* packet arrived with bad crc */
	Berror=	1<<1,	/* some other form of error */
	Bowned=	1<<2,	/* block owned by someone; don't free in freeb */
};

enum {
	CHANEND,
	CHANSND,
	CHANRCV,
	CHANNOP,
	CHANNOBLK,
};

struct Alt {
	byte	op;
	Channel*	c;
	void*	v;
	byte	sel;
};

struct Channel
{
//	int	(*send)(Channel*, int);
//	int	(*recv)(Channel*, int*);
	Proc*	s;
	Proc*	r;
};

struct Queue {
	byte	nb;	/* blocks on queue */
	byte	hiwat;	/* block limit for queue */
	byte	state;
	Block*	first;
	Block*	last;

	Block*	(*get)(Queue*);
	void	(*put)(Queue*, Block*);
	byte	(*ctl)(Queue*, char*);
	void	(*canput)(Queue*);	/* called by consumer when blocked queue drains */
	void	(*canget)(Queue*);	/* called by producer when starved queue fed */

	union {
		void	*d;
		int	i;
		byte	b;
	};
	Queue*	next;	/* when in stream */
	Lock;
};

/* queue state bits,  Qmsg, Qcoalesce, and Qkick can be set in qopen */
enum
{
	/* Queue.state */
	Qstarve		= (1<<0),	/* consumer starved */
	Qflow		= (1<<1),	/* producer flow controlled */
	Qclosed		= (1<<2),	/* queue has been closed/hungup */
	Qcoalesce		= (1<<3),		/* coalesce packets on read/write */
};

#define	QDATA(T, q)	((T)(q)->data)
#define	putnext(q, b)	((q)->next->put)((q), (b))

struct Qproto {
	byte	hiwat;
	void	(*put)(Queue*, Block*);
	byte	(*ctl)(Queue*, char*);
	Block*	(*get)(Queue*);
	void	(*canput)(Queue*);
	void	(*canget)(Queue*);
};

struct Task {
	void	(*f)(void*);
	void	*arg;
	Task*	sched;
	byte	state;
};

struct Proc {
	Task;
	Label;
	Proc*	qnext;	/* in qlock */
	void*	ptr;	/* Alt* if Altq, int* if Recvq or Sendq */
	byte	stack[STACKSIZE];
};

enum {
	/* Task.state */
	Inactive,
	Blocked,
	Wakeme,
	Altq,
	Sendq,
	Recvq,
	Ready,
	Transient,
	Pending
};

/*
 * flash file system
 */
struct File {
	char*	name;
	ushort	here;	/* current logical page */
	ushort	bytes;	/* current page offset (writing); limit (reading) */
	ushort	initial;	/* initial page */
	ushort	page;	/* current page */
	ushort	next;	/* next page */
	byte	dirslot;
};

enum {
	Unbuffered=	1<<15	/* flashread to bypass buffer if not already cached */
};

enum {
	Noroute=	0xFFFF
};

#pragma	incomplete	CQ
#pragma	incomplete	Timer

enum {
	Enodev = 1,
	Enoctl,
	Ebadctl,

	Maxerrno
};

extern	void	activate(Task*);
extern	Block*	allocb(int);
extern	void	blockinit(void);
extern	Block*	bprefix(Block*, int);
extern	short	bticks(Block*);
extern	byte	canqlock(QLock*);
extern	ushort	crc16block(byte*, short);
extern	ushort	crc16byte(ushort, byte);
extern	Proc*	curproc(void);
extern	char*	donprint(char*, char*, char*, void*);
extern	byte	eepromread(ushort);
extern	void	eepromwrite(ushort, byte);
extern	uint	eepromget2(ushort);
extern	ulong	eepromget4(ushort);
extern	void	eepromput2(ushort, uint);
extern	void	eepromput4(ushort, ulong);
extern	void	free(void*);
extern	void	freeb(Block*);
extern	void	freeblist(Block*);
extern	ushort	get2(byte*);
extern	ushort	getcallerpc(void);
extern	ushort*	getsp(void);
extern	void	gotolabel(Label*);
extern	void	idlehands(void);
extern	void	ilock(Lock*);
extern	void	iunlock(Lock*);
extern	void	ledinit(void);
extern	void	ledset(byte);
extern	void	ledtoggle(byte);
extern	void*	malloc(uint);
extern	void	mallocinit(void);
extern	void*	memccpy(void*, void*, int, uint);
extern	void*	memchr(void*, int, uint);
extern	int	memcmp(void*, void*, uint);
extern	void*	memmove(void*, void*, uint);
extern	void*	memset(void*, int, uint);
extern	void	moteid(void);
extern	void	panic(char*, ...);
extern	void	pexit(void);
extern	int	print(char*, ...);
extern	void	procinit(void);
extern	void	put2(byte*, ushort);
extern	void	putc(byte);
extern	void	puts(char*);
extern	Queue*	qalloc(Qproto*);
extern	byte	qcanget(void*);
extern	byte	qcanput(void*);
extern	void	qclose(Queue*);
extern	int	qctl(Queue*, char*);
extern	void	qdisable(Queue*);
extern	void	qenable(Queue*);
extern	void	qenablehi(Queue*);
extern	void	qflush(Queue*);
extern	void	qfree(Queue*);
extern	Block*	qget(Queue*);
extern	int	qgetc(Queue*);
extern	byte	qblen(Queue*);
extern	void	qlock(QLock*);
extern	void	qput(Queue*, Block*);
extern	void	qputback(Queue*, Block*);
extern	byte	qputc(Queue*, byte);
extern	void	queueinit(void);
extern	void	qunlock(QLock*);
extern	void	ready(Proc*);
extern	byte	return0(void*);
extern	void	sched(void);
extern	byte	setlabel(Label*);
extern	ushort	shortid(void);
extern	void	sleep(Rendez*, byte (*)(void*), void*);
extern	int	snprint(char*, int, char*, ...);
extern	Proc*	spawn(void (*)(void*), void*);
extern	byte	splhi(void);
extern	byte	spllo(void);
extern	void	splx(byte);
extern	void	stackdump(void);
extern	int	stkfp(void);
extern	int	stkpc(void);
extern	void	stoptimer(void (*)(void*,int), void*, Timer*);
extern	char*	strchr(char*, byte);
extern	int	strcmp(char*, char*);
extern	int	strlen(char*);
extern	int	strncmp(char*, char*, int);
extern	char*	strncpy(char*, char*, int);
extern	void	timein(void);
extern	Timer*	timeout(void (*)(void*, int), void*, long);
extern	int	timercheck(void);
extern	void	timerinit(void);
extern	void	transient(Task*, void (*)(void*), void*);
extern	void	uart0init(void);
extern	void	uartputc(int);
extern	void	uartputs(char*, int);
extern	void	uartputi(int);
extern	void	uartputl(long);
extern	Block*	uartread(byte);
extern	void	uartwrite(Block*);
extern	void	wait250ns(void);
extern	void	waitusec(ushort);
extern	void	wakeup(Rendez*);

	long	clockticks;
	ushort	fastticks;	/* must be splhi when read */

	byte	myid[8];	/* dallas ID after moteid() */
	
/* diagnostics */
enum{
	Dbattery,
	Dbias,
	Dcantwrite,
	Ddeferred,
	Di2c,
	Dlostmate,
	Dnoblk,
	Dnoid,
	Dnomem,
	Dnoroute,
	Dpanic,
	Dpot,
	Dstray,
	Dzed,
};

void	diaginit(void);
void	diag(int);
void	diagv(int, int);
void	diagput(char*);
void	diagsend(void);

extern	ulong	seconds(void);
extern	void	nextsecond(void);

/*
 * circular queues
 */
extern	CQ*	cqalloc(ushort, byte);
extern	void	cqflush(CQ*);
extern	ushort	cqget(CQ*, byte);
extern	ushort	cqnlost(CQ*);
extern	byte	cqnonempty(void*);
extern	byte	cqput(CQ*, ushort);

/*
 * channels
 */
extern	byte	alt(Alt*);
extern	byte	nbrecv(Channel*, int*);
extern	void*	nbrecvp(Channel*);
extern	byte	nbsend(Channel*, int);
extern	byte	nbsendp(Channel*, void*);
extern	int	recv(Channel*);
extern	void*	recvp(Channel*);
extern	void	send(Channel*, int);
extern	void	sendp(Channel*, void*);

/*
 * io support
 */
extern	void	clockinit(void);
extern	void	clockon(void);
extern	void	clockoff(void);
extern	void	portinit(void);
extern	void	portpoweron(byte);
extern	void	portpoweroff(byte);

extern	void	i2cinit(void);
extern	void	i2cdisable(void);
extern	int	i2crecv(byte, byte*, int);
extern	int	i2csend(byte, byte*, int);

extern	void	ds2401init(void);
extern	byte	ds2401readid(byte*, byte);

extern	void	spipause(void);
extern	void	spislave(void);
extern	void	spirx(void);
extern	void	spitx(void);
#define	spigetc()	(iomem.spdr)
#define	spiputc(x)	(iomem.spdr = (x))
extern	void	spioff(void);

extern	byte	cc1kgetlock(void);
extern	byte	cc1kgetreg(byte);
extern	void	cc1kbias(byte);
extern	void	cc1kcal(void);
extern	void	cc1kcal(void);
extern	void	cc1kdump(void);
extern	void	cc1kdump(void);
extern	void	cc1kload(void);
extern	void	cc1kputreg(byte, byte);
extern	void	cc1kreset(void);
extern	void	cc1krx(void);
extern	void	cc1ktx(void);
extern	void cc1ksleep(void);
extern	void cc1kwake(void);

extern	void	radioinit(void);
extern	byte	radiosend(Block*);
extern	byte	radiosense(Rendez*);
extern	byte	radiobusy(void);
extern	void	radiosleep(void);
extern	void	radioidle(void);
extern	Block*	radiorecv(Rendez*);
extern	void	radiorssi(void);

extern	void	flashinit(void);
extern	void	flashenable(void);
extern	void	flashdisable(void);
extern	void	flashread(ushort, ushort, void*, int);
extern	byte	flashpagewrite(ushort, void*, int, byte*);

extern	void	lfsinit(void);
extern	File*	fopen(char*);
extern	File*	fcreate(char*);
extern	void	fclose(File*);
extern	int	fread(File*, void*, int, long);
extern	char	fwrite(File*, void*, int);
extern	byte	fsetpos(File*, ushort);
extern	void	fchop(File*);
extern	void	remove(char*);

/*
 * sensor board
 */
extern	void	photodisable(void);
extern	void	photoenable(void);
extern	ushort	photoread(void);
extern	void	tempdisable(void);
extern	void	tempenable(void);
extern	ushort	tempread(void);
extern	void	potenable(void);
extern	void	potdisable(void);
extern	byte	potset(byte);
extern	byte	potget(void);

extern	ushort	rand15(void);
extern	void	srand15(ushort);

/*
 * mac.c
 */
extern	void	macinit(ushort, void (*f)(ushort));
extern	Block*	macread(ushort*, ushort*);
extern	void	macwrite(ushort, Block*);
extern	byte	ismate(ushort);
extern	byte	maccanwrite(void);
extern	void	mactimer(void);
extern	void	mactimeron(void);
extern	void	mactimeroff(void);
extern	void	macwaitslot(ushort);
extern	void	macwaitrx(void);

/*
 * bat.c
 */
extern	int	batterylevel(void);
extern	int	rawbatterylevel(void);

/*
 * sink.c
 */
extern	void	newsink(Block*);
extern	void	noroutemate(ushort);
extern	ushort	sinkroute(void);
extern	byte	issink(void);
extern	void	addsink(Block*, ushort);
extern	void	setsinkhere(ushort);

/*
 * woof.c
 */
extern	void	watchdogset(ushort);
extern	void	watchdogoff(void);
extern	void	watchdogpat(void);

/*
 * fft256.c
 */
extern void fftproc(void*);
