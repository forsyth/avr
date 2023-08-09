#include "u.h"
#include "mem.h"
#include "os.h"
#include "adc.h"

#define DPRINT if(0)print

#define Fac	66
#define LogFac	Log66
#define Wscale	(21846/Fac)

int	_fftop(int, int);
long	_fftsq(int);
void	_fft(int*, int*);

#define Ndb	2	/* power of 2 > 1 and <= 256 */

#define Pow	10
#define Scale	(1<<Pow)
#define Logscale	1000
#define Logrpt	0	/* was 2 was 4 */
#define Rpt	(1<<Logrpt)

#define U(x)	((ulong)(x))
#define R(x, y)	_fftop(x, y)
#define S(x)	(U(_fftsq(x)+Rpt/2)>>Logrpt)
#define T(x)	(U((x)+Scale/2)>>Pow)

/* Logx = 10*log10(x) */
#define Log2	3
#define Log3	5
#define Log11	10
#define Log331	25

#define Log6	(Log2+Log3)
#define Log66	(Log6+Log11)
#define Log21846	(Log66+Log331)

#define Dbmax	100			/* decibel level at top of scale */
#define Dbmin	(Dbmax-60)	/* decibel level at zero */
#define Dbadj	(Dbmax+3*Log2-LogFac)	/* decibel level adjustment */

static int sintab[] = {
	0,		/* PI */
	1024,	/* PI/2 */
	724,		/* PI/4 */
	392,		/* PI/8 */
	200,		/* PI/16 */
	100,		/* PI/32 */
	50,		/* PI/64 */
	25,		/* PI/128 */
	13,		/* PI/256 */
	6,		/* PI/512 */
	3,		/* PI/1024 */
};

static int logtab[] = {
	3162,	/* 10**1/2 */
	1778,	/* 10**1/4 */
	1334,	/* 10**1/8 */
	1155,	/* 10**1/16 */
	1075,	/* 10**1/32 */
	1037,	/* 10**1/64 */
	1018,	/* 10**1/128 */
	1009,	/* 10**1/256 */
	1005,	/* 10**1/512 */
	1002,	/* 10**1/1024 */
};

/* assumes n=256, fc = 35446/2 */
static int atab[] = {
	/* 0, */	/* f = 0 */
	33,		/* f = 138 */
	172,		/* f = 277 */
	363,		/* f = 415 */
	560,		/* f = 554 */
	736,		/* f = 692 */
	883,		/* f = 831 */
	1001,		/* f = 969 */
	1095,		/* f = 1108 */
	1168,		/* f = 1246 */
	1224,		/* f = 1385 */
	1268,		/* f = 1523 */
	1301,		/* f = 1662 */
	1326,		/* f = 1800 */
	1344,		/* f = 1938 */
	1357,		/* f = 2077 */
	1366,		/* f = 2215 */
	1370,		/* f = 2354 */
	1372,		/* f = 2492 */
	1371,		/* f = 2631 */
	1368,		/* f = 2769 */
	1363,		/* f = 2908 */
	1356,		/* f = 3046 */
	1348,		/* f = 3185 */
	1339,		/* f = 3323 */
	1328,		/* f = 3462 */
	1317,		/* f = 3600 */
	1304,		/* f = 3738 */
	1291,		/* f = 3877 */
	1277,		/* f = 4015 */
	1262,		/* f = 4154 */
	1247,		/* f = 4292 */
	1232,		/* f = 4431 */
	1216,		/* f = 4569 */
	1199,		/* f = 4708 */
	1183,		/* f = 4846 */
	1166,		/* f = 4985 */
	1148,		/* f = 5123 */
	1131,		/* f = 5262 */
	1113,		/* f = 5400 */
	1096,		/* f = 5538 */
	1078,		/* f = 5677 */
	1060,		/* f = 5815 */
	1042,		/* f = 5954 */
	1024,		/* f = 6092 */
	1006,		/* f = 6231 */
	989,		/* f = 6369 */
	971,		/* f = 6508 */
	953,		/* f = 6646 */
	935,		/* f = 6785 */
	918,		/* f = 6923 */
	900,		/* f = 7062 */
	883,		/* f = 7200 */
	866,		/* f = 7338 */
	849,		/* f = 7477 */
	832,		/* f = 7615 */
	816,		/* f = 7754 */
	799,		/* f = 7892 */
	783,		/* f = 8031 */
	767,		/* f = 8169 */
	751,		/* f = 8308 */
	736,		/* f = 8446 */
	721,		/* f = 8585 */
	705,		/* f = 8723 */
	691,		/* f = 8862 */
	676,		/* f = 9000 */
	662,		/* f = 9138 */
	647,		/* f = 9277 */
	633,		/* f = 9415 */
	620,		/* f = 9554 */
	606,		/* f = 9692 */
	593,		/* f = 9831 */
	580,		/* f = 9969 */
	568,		/* f = 10108 */
	555,		/* f = 10246 */
	543,		/* f = 10385 */
	531,		/* f = 10523 */
	519,		/* f = 10661 */
	508,		/* f = 10800 */
	496,		/* f = 10938 */
	485,		/* f = 11077 */
	475,		/* f = 11215 */
	464,		/* f = 11354 */
	454,		/* f = 11492 */
	444,		/* f = 11631 */
	434,		/* f = 11769 */
	424,		/* f = 11908 */
	414,		/* f = 12046 */
	405,		/* f = 12185 */
	396,		/* f = 12323 */
	387,		/* f = 12461 */
	379,		/* f = 12600 */
	370,		/* f = 12738 */
	362,		/* f = 12877 */
	354,		/* f = 13015 */
	346,		/* f = 13154 */
	338,		/* f = 13292 */
	331,		/* f = 13431 */
	323,		/* f = 13569 */
	316,		/* f = 13708 */
	309,		/* f = 13846 */
	302,		/* f = 13985 */
	296,		/* f = 14123 */
	289,		/* f = 14261 */
	283,		/* f = 14400 */
	276,		/* f = 14538 */
	270,		/* f = 14677 */
	264,		/* f = 14815 */
	259,		/* f = 14954 */
	253,		/* f = 15092 */
	247,		/* f = 15231 */
	242,		/* f = 15369 */
	237,		/* f = 15508 */
	231,		/* f = 15646 */
	226,		/* f = 15785 */
	222,		/* f = 15923 */
	217,		/* f = 16061 */
	212,		/* f = 16200 */
	208,		/* f = 16338 */
	203,		/* f = 16477 */
	199,		/* f = 16615 */
	194,		/* f = 16754 */
	190,		/* f = 16892 */
	186,		/* f = 17031 */
	182,		/* f = 17169 */
	178,		/* f = 17308 */
	175,		/* f = 17446 */
	171,		/* f = 17585 */
	167,		/* f = 17723 */
};

static uchar rev128[] = {
	2*0,
	2*64,
	2*32,
	2*96,
	2*16,
	2*80,
	2*48,
	2*112,
	2*8,
	2*72,
	2*40,
	2*104,
	2*24,
	2*88,
	2*56,
	2*120,
	2*4,
	2*68,
	2*36,
	2*100,
	2*20,
	2*84,
	2*52,
	2*116,
	2*12,
	2*76,
	2*44,
	2*108,
	2*28,
	2*92,
	2*60,
	2*124,
	2*2,
	2*66,
	2*34,
	2*98,
	2*18,
	2*82,
	2*50,
	2*114,
	2*10,
	2*74,
	2*42,
	2*106,
	2*26,
	2*90,
	2*58,
	2*122,
	2*6,
	2*70,
	2*38,
	2*102,
	2*22,
	2*86,
	2*54,
	2*118,
	2*14,
	2*78,
	2*46,
	2*110,
	2*30,
	2*94,
	2*62,
	2*126,
	2*1,
	2*65,
	2*33,
	2*97,
	2*17,
	2*81,
	2*49,
	2*113,
	2*9,
	2*73,
	2*41,
	2*105,
	2*25,
	2*89,
	2*57,
	2*121,
	2*5,
	2*69,
	2*37,
	2*101,
	2*21,
	2*85,
	2*53,
	2*117,
	2*13,
	2*77,
	2*45,
	2*109,
	2*29,
	2*93,
	2*61,
	2*125,
	2*3,
	2*67,
	2*35,
	2*99,
	2*19,
	2*83,
	2*51,
	2*115,
	2*11,
	2*75,
	2*43,
	2*107,
	2*27,
	2*91,
	2*59,
	2*123,
	2*7,
	2*71,
	2*39,
	2*103,
	2*23,
	2*87,
	2*55,
	2*119,
	2*15,
	2*79,
	2*47,
	2*111,
	2*31,
	2*95,
	2*63,
	2*127,
};

static int
x10log10(ulong v)
{
	byte i;
	int d, r, *p;
	ulong l;

	if(v == 0)
		return 0;
	r = 0;
	while(v >= 1000000U){
		v = (v+5)/10;
		r += Logscale;
	}
	v *= Logscale;
	while(v >= 10*Logscale){
		v = (v+5)/10;
		r += Logscale;
	}
	while(v < Logscale){
		v *= 10;
		r -= Logscale;
	}
	d = Logscale/2;
	p = logtab;
	for(i = 0; i < sizeof(logtab)/sizeof(logtab[0]); i++){
		if((l = *p++) <= v){
			v = (Logscale*v+(l>>1))/l;
			r += d;
		}
		d >>= 1;
	}
	return (r + Logscale/20)/(Logscale/10);			
}

static void
fftr(int *f)
{
	byte i;
	int a, b, c, s, t, ep, em, op, om;
	int *fp, *fq;

	_fft(f, sintab);
	t = sintab[7+1];
	a = 2*R(t, t);
	b = sintab[7];
	c = Scale-a;
	s = b;
	fp = f;
	fq = fp+256;
	t = *fp;
	ep = fp[1];
	*fp++ = t+ep;
	*fp++ = t-ep;
	// for(i = 2; i < 128; i += 2){
	for(i = 1; i < 64; i++){
		fq--;
		ep = *fp + fq[-1];
		em = *fp - fq[-1];
		op = fp[1] + *fq;
		om = fp[1] - *fq;
		t = R(c, op)+R(s, em);
		*fp++ = (ep+t)>>1;
		fq[-1] = (ep-t)>>1;
		t = R(c, em)-R(s, op);
		*fp++ = (om-t)>>1;
		*fq-- = (-om-t)>>1;
		t = c;
		c -= R(a, t)+R(b, s);
		s += R(b, t)-R(a, s);
	}
}

static void
spectrum(int *f, ulong *p)
{
	byte i;
	int t, *fp;

	fftr(f);
	fp = f;
	*p++ += S(*fp++);
	t = *fp++;
	for(i = 1; i < 128; i++)
		*p++ += 2*(S(*fp++)+S(*fp++));
	*p += S(t);
}

static ulong
xform(int *f, int bias)
{
	int j, k, v, *fp;
	uchar *rp;
	ulong sum;

	fp = f;
	sum = 0;
	for(j = 0; j < 256; j++){
		v = *fp;
		sum += v;
		v -= bias;
		if(v >= 0)
			v >>= 1;
		else
			v = (v+1)>>1;
		*fp++ = v;
	}
	fp = f;
	for(j = 0; j < 128; j++)
		*fp++ = (j**fp+64)>>7;
	for(j = 128; j < 256; j++)
		*fp++ = ((256-j)**fp+64)>>7;
	rp = rev128;
	for(j = 0; j < 256; j += 2){
		k = *rp++;
		if(k > j){
			v = f[j]; f[j] = f[k]; f[k] = v;
			v = f[j+1]; f[j+1] = f[k+1]; f[k+1] = v;
		}
	}
	return sum;
}

static ulong
aweight(ulong *p)
{
	byte i;
	int a, *t;
	ulong v, w;

	w = 0;
	p++;
	t = atab;
	for(i = 1; i <= 128; i++){
		a = *t++;
		v = *p++;
		if(v >= U(1)<<21)
			w += U(65536U/Scale)*a*(v>>16)+T(a*(v&0xffffU));
		else
			w += T(a*v);
		/* w += T((*p++)*(*t++)); */
	}
	return w;
}

static byte
setpot(int vr, int bias, byte potv)
{
	byte b;
	int vm;
	long lb;

	if(bias > 512)
		bias = 1024-bias;
	vm = ((long)vr*(long)bias+512)/1024;
	lb = (40*(long)vm-24768+1875/2)/1875;
	b = lb;
	if(lb < 0)	/* probably faulty */
		b = 0;
	if(lb > 255)	/* ditto */
		b = 255;
	if(b != potv){
		potset(b);
		diagv(Dpot, b);
	}
	return b;
}

/* vbias*1024 == vref*adc bias */
void
fftproc(void*)
{
	int db, bias, vref, t;
	ulong w;
	int *f;
	ulong *p, *q;
	Adc *a;
	ushort id, ss;
	byte s, potv;
	byte i, j, k, v;
	ulong sum;
	Block *b;
	
	id = shortid();
	bias = 480;
	vref = 3200;
	potv = 255;
	ss = adcscale(35446U, &s);
	if(ss == 0)
		panic("scale");
	portpoweron(6);
	potenable();
	potv = setpot(vref, bias, potv);
	nextsecond();
	nextsecond();
	a = adcalloc(ADC2, s);
	i = 0;
	b = nil;
	f = malloc(256*sizeof(int));
	p = malloc(129*sizeof(ulong));
	if(f == nil || p == nil)
		panic("fft");
	/* first loop establishes actual vref, bias and potv */
	for(;;){
		nextsecond();
		macwaitslot(85);
		adcacquire(a, 1);
		adcenable(a);
		memset(p, 0, 129*sizeof(ulong));
		sum = 0;
		t = seconds();
		for(j = 0; j < Rpt; j++){
			adcsample(a, 2, (ushort*)f);	/* first conversion takes 25 cycles not 13 - throw away */
			clockoff();
			mactimeroff();
			adcsample(a, 256, (ushort*)f);
			mactimeron();
			clockon();
			s = splhi();
			for(k = 0; k < 7; k++)
				mactimer();	/* compensate */
			if((k = i%9) == 0 || k == 5)
				mactimer();	/* another 2/9 */
			splx(s);
			sum += xform(f, bias);
			spectrum(f, p);
		}
		sum = (sum+128*Rpt)/(256*Rpt);
		adcdisable();
		adcrelease(a);
		for(q = p, j = 0; j <= 128; j++)
			*q++ = (*q+Wscale/2)/Wscale;
		w = aweight(p);
		if(w == 0)
			db = Dbmin;
		else{
			db = x10log10(w)+Dbadj;
			if(bias <= 512)
				db -= 2*x10log10(bias);
			else
				db -= 2*x10log10(1024-bias);
		}
		if(!i && (ss = batterylevel()) != vref){
			vref = ss;
			bias = -1;
			diagv(Dbattery, vref);
		}
		if(sum != bias){
			bias = sum;
			potv = setpot(vref, bias, potv);
			diagv(Dbias, bias);
		}
		if(vref < 2700)
			db = 0;
		// uartputl(0xfedcba98L);
		// for(q = p, j = 0; j <= 128; j++)
		//	uartputl(*q++);
		// uartputi(0x7654);
		// uartputc(db);
		// if(i < Ndb) { if(i == 0) diagsend(); i++; continue; }
		v = i&(Ndb-1);
		if(v == 0){
			b = allocb(4+Ndb);
			if(b != nil){
				put2(b->wp, id);
				put2(b->wp+2, t);
				b->wp[4] = db;
				b->wp += 5;
			}
		}
		else{
			if(b != nil){
				b->wp[0] = db;
				b->wp++;
				if(v == Ndb-1){
					if((ss = sinkroute()) != Noroute && maccanwrite())
						macwrite(ss, b);
					else{
						freeb(b);
						diag(ss ==Noroute ? Dnoroute : Dcantwrite);
					}
					b = nil;
				}
			}
		}
		i++;
		if((t&0xfff) == 0)	/* roughly every 68 minutes */
			diagput(nil);
	}
	potdisable();
	portpoweroff(6);
}
