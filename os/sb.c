#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "adc.h"

/*
 * mote sensor board
 */


enum {
	Temp_adc=	ADC1,
	E_temp_pwr_o=	1<<6,	/* docs say INT2 */
	Photo_adc=	ADC1,
	E_photo_pwr_o=	1<<5,	/* docs say INT1 */
};

static Adc *tpadc;	/* shared */

void
tempenable(void)
{
	byte s;

	if(tpadc == nil)
		tpadc = adcalloc(Temp_adc, Scale64);
	s = splhi();
	iomem.porte |= E_temp_pwr_o;
	iomem.ddre |= E_temp_pwr_o;
	splx(s);
}

void
tempdisable(void)
{
	byte s;

	s = splhi();
	iomem.porte &= ~E_temp_pwr_o;
	iomem.ddre &= ~E_temp_pwr_o;
	splx(s);
}

ushort
tempread(void)
{
	return adcread1(tpadc);
}


void
photoenable(void)
{
	byte s;

	if(tpadc == nil)
		tpadc = adcalloc(Photo_adc, Scale64);
	s = splhi();
	iomem.porte |= E_photo_pwr_o;
	iomem.ddre |= E_photo_pwr_o;
	splx(s);
}

void
photodisable(void)
{
	byte s;

	s = splhi();
	iomem.porte &= ~E_photo_pwr_o;
	iomem.ddre &= ~E_photo_pwr_o;
	splx(s);
	adcdisable();
}

ushort
photoread(void)
{
	return adcread1(tpadc);
}

/*
 * digital pot.
 */

enum {
	Potaddr=	0x58|(1<<1),	/* on i2c */
	Pot_pwr_port=	3,
};

void
potenable(void)
{
	i2cinit();
	portpoweron(Pot_pwr_port);
}

void
potdisable(void)
{
	portpoweroff(Pot_pwr_port);
	i2cdisable();
}

byte
potset(byte v)
{
	byte b[2];
	int n;

	b[0] = 0;
	b[1] = v;
	n = i2csend(Potaddr, b, 2);
	/* print("n=%d\n", n); */
	return n == 2;
}

byte
potget(void)
{
	byte b;
	int n;

	n = i2crecv(Potaddr, &b, 1);
	/* print("n=%d %.2ux[%c]\n", n, b, b); */
	return b;
}
