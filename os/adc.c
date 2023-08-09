#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "adc.h"

/*
 * basic ADC interface
 */

enum{
	/* control bits in adcsra */
	Enable=	1<<7,
	Start=	1<<6,
	Freerun=	1<<5,
	Iflag=	1<<4,
	Ienable=	1<<3,
};

struct {
	QLock;
	Adc*	a;	/* current owner */
} adc;

static ushort avp[2];

void
adcinit(void)
{
}

ushort
adcscale(ushort sps, byte *sp)
{
	ushort d, u;
	byte s;

	d = CLOCKFREQ/((ulong)sps * AdcNormTime);
	u = 2;
	for(s = Scale2; (u<<=1) <= d;)	/* was <, probably should be < */
		s++;
	if(s > Scale128)
		return 0;
	*sp = s;
	return CLOCKFREQ/((1<<s)* AdcNormTime);
}

Adc*
adcalloc(byte dev, byte scale)
{
	Adc *a;

	a = malloc(sizeof(*a));
	if(a == nil)
		return nil;
	memset(a, 0, sizeof(*a));
	a->dev = dev;
	a->scale = scale;
	a->vp = nil;
	return a;
}

byte
adcisbusy(void)
{
	return adc.a != nil;
}

byte
adcacquire(Adc *a, byte wait)
{
	if(!wait && adc.a != nil)
		return 0;
	qlock(&adc);
	adc.a = a;
	iomem.admux = a->dev;
	iomem.adcsra = Enable | a->scale | Iflag;
	return 1;
}

void
adcrelease(Adc *a)
{
	if(adc.a != a)
		panic("adcrelease");
	adc.a = nil;
	iomem.adcsra = Iflag;	/* disable and clear interrupt */
	qunlock(&adc);
}

static byte
adcdone(void *v)
{
	return ((Adc*)v)->nsamp == 0;
}

ushort
adcread(Adc *a)
{
	byte s;
	ushort v;

	if(adc.a != a)
		panic("adcread");
	a->f = nil;
	a->nsamp = 1;
	a->vp = &v;
	s = splhi();
	iomem.adcsra |= Start | Ienable;
	splx(s);
	sleep(&a->r, adcdone, a);
	return v;
}

void
adcsample(Adc *a, ushort n, ushort *vp)
{
	byte s;

	if(adc.a != a)
		panic("adcsample");
	if(n == 0)
		return;
	a->f = nil;
	a->nsamp = n;
	a->vp = vp;
	s = splhi();
	if(n > 1)
		iomem.adcsra |= Freerun;
	iomem.adcsra |= Start | Ienable;
	splx(s);
	sleep(&a->r, adcdone, a);
}

/*
 * start a sample but don't wait for it;
 * used in non-adc interrupts to start a new sample
 */
void
adctrigger(Adc *a, void (*f)(ushort))
{
	if(adc.a != a)
		panic("adctrigger");
	a->f = f;
	a->nsamp = 2;	/* not 1: prevent wakeup in adcintr */
	a->vp = avp;	/* buffer for interrupt routine */
	iomem.adcsra = Enable | a->scale | Start | Ienable | Iflag;	/* atomic */
}

/*
 * shorthand for complete cycle; don't use for several readings in a row
 */
ushort
adcread1(Adc *a)
{
	byte s;
	ushort v;

	a->f = nil;
	a->nsamp = 1;
	a->vp = &v;
	adcacquire(a, 1);
	s = splhi();
	iomem.adcsra |= Start | Ienable;
	splx(s);
	sleep(&a->r, adcdone, a);
	adcrelease(a);
	return v;
}

/*
void
adcintr(void*)
{
	Adc *a;
	ushort v;

	a = adc.a;
	if(a == nil)
		return;
	v = adcgetw();
	*a->vp++ = v;
	if(a->f != nil)
		a->f(v);
	if(--a->nsamp == 0)
		wakeup(&a->r);
}
*/

/*
 * can be called by current adc owner for power saving
 */

void
adcdisable(void)
{
	iomem.adcsra = Iflag;
}

void
adcenable(Adc *a)
{
	byte s;

	s = splhi();
	iomem.admux = a->dev;
	iomem.adcsra = Enable | a->scale | Iflag;
	splx(s);
}
