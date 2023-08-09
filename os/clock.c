#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

// Usage is Clock.setRate(TOS_InPS, TOS_SnPS)
// the following setting is for Atmega128L mica motes only. 
enum {
  TOS_I1000PS = 32,  TOS_S1000PS = 1,
  TOS_I100PS  = 40,  TOS_S100PS  = 2,
  TOS_I10PS   = 101, TOS_S10PS   = 3,
  TOS_I1024PS = 0,   TOS_S1024PS = 3,
  TOS_I512PS  = 1,   TOS_S512PS  = 3,
  TOS_I256PS  = 3,   TOS_S256PS  = 3,
  TOS_I128PS  = 7,   TOS_S128PS  = 3,
  TOS_I64PS   = 15,  TOS_S64PS   = 3,
  TOS_I32PS   = 31,  TOS_S32PS   = 3,
  TOS_I16PS   = 63,  TOS_S16PS   = 3,
  TOS_I8PS    = 127, TOS_S8PS    = 3,
  TOS_I4PS    = 255, TOS_S4PS    = 3,
  TOS_I2PS    = 15 , TOS_S2PS    = 7,
  TOS_I1PS    = 31 , TOS_S1PS    = 7,
  TOS_I0PS    = 0,   TOS_S0PS    = 0
};
enum {
  DEFAULT_SCALE=3, DEFAULT_INTERVAL=127
};

enum {
	/* assr */
	AS0=	1<<3,	/* asynchronous operation */
	TCN0UB=	1<<2,	/* tcnt0 update busy */
	OCR0UB=	1<<1,	/* ocr0 update busy */
	TCR0UB=	1<<0,	/* tcr0 update busy */

	/* timsk */
	OCIE0=	1<<1,	/* output compare interrupt enable */
	TOIE0=	1<<0,	/* overflow interrupt enable */

	/* tifr */
	OCF0=	1<<1,	/* output compare flag */
	TOV0=	1<<0,	/* overflow flag */

	/* tccr0 */
	Normal=	0,	/* normal mode */
	PWM=	1<<6,	/* PWM, phase correct */
	CTC=	1<<3,	/* clear timer on compare */
	FastPWM=	(1<<3)|(1<<6),	/* fast PWM */
	Com0=	0<<4,	/* normal port, OC0 disconnected */
	Com1=	1<<4,	/* toggle OC0 on match */
	Com2=	2<<4,	/* clear OC0 on match */
	Com3=	3<<4,	/* set OC0 on match */
};

static ulong ticked;

ulong
seconds(void)
{
	byte s;
	ulong v;

	s = splhi();
	v = ticked;
	splx(s);
	return v;
}

static Rendez tickr;

void
nextsecond(void)
{
	sleep(&tickr, return0, nil);
}

void
timer0intr(void)
{
//uartputs("xxx\n", -1);
	ledtoggle(Yellow);
//uartputc('!');
	ticked++;
	if(tickr.p != nil)
		wakeup(&tickr);
}

static byte mscale;
static byte nextscale;
static byte minterval;

static void
setrate(byte interval, byte scale)
{
	byte s;

	/* see ATmega128, p. 103 */
	s = splhi();
	iomem.timsk &= ~(TOIE0|OCIE0);
	iomem.assr |= AS0;	/* use 32k clock */
	iomem.tcnt0 = 0;
	iomem.ocr0 = interval;
	iomem.tccr0 = scale | CTC;
	while(iomem.assr & (TCN0UB|OCR0UB|TCR0UB)){
		/* wait for registers to update */
	}
	iomem.tifr &= ~(OCF0|TOV0);	/* clear existing interrupt flags */
	iomem.timsk |= OCIE0;
	splx(s);
}

void
clockinit(void)
{
	mscale = DEFAULT_SCALE;
	minterval = DEFAULT_INTERVAL;
	setrate(31, 7);
}

void
clockoff(void)
{
	byte s;

	s = splhi();
	iomem.timsk &= ~OCIE0;
	splx(s);
}

void
clockon(void)
{
	byte s;

	s = splhi();
	iomem.timsk |= OCIE0;
	splx(s);
}
