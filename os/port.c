#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

void
portinit(void)
{
	iomem.ddrc = 0xFF;	/* all output */
	iomem.portc = 0x00;	/* all off */
//	iomem.ddre = 0xF0;	/* output */
//	iomem.porte = 0x00;	/* off */
}

void
portpoweron(byte b)
{
	byte s;

	s = splhi();
	iomem.portc |= 1<<b;
	splx(s);
}

void
portpoweroff(byte b)
{
	byte s;

	s = splhi();
	iomem.portc &= ~(1<<b);
	splx(s);
}
