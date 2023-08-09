#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "adc.h"

/*
 * battery status
 */

enum {
	A_bat_mon_o=	1<<5,
	Bat_adc=	ADC7
};

static Adc *batadc;

int
batterylevel(void)
{
	int ac;

	ac = rawbatterylevel();
	if(ac == 0)
		ac = 1;
	return (1223L*1024L+ac/2)/ac;
}

int
rawbatterylevel(void)
{
	byte s;
	int v;

	if(batadc == nil)
		batadc = adcalloc(Bat_adc, Scale64);

	s = splhi();
	iomem.porta |= A_bat_mon_o;
	iomem.ddra |= A_bat_mon_o;
	splx(s);
	waitusec(20);	/* should be enough start up time */

	v = adcread1(batadc);

	s = splhi();
	iomem.ddra &= ~A_bat_mon_o;
	iomem.porta &= ~A_bat_mon_o;
	splx(s);
	return v;
}
