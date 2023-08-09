#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

/*
 * internal EEPROM
 */

enum {
	Eere=	1<<0,	/* EEPROM read enable */
	Eewe=	1<<1,	/* EEPROM write enable */
	Eemwe=	1<<2,	/* EEPROM master write enable */
	Eerie=	1<<3,	/* EEPROM ready interrupt enable */
};

extern	void	eestartw(void);

static struct {
	Rendez	r;
} eeio;

static byte
eedone(void*)
{
	if(iomem.eecr & Eewe){
		iomem.eecr |= Eerie;
		return 0;
	}
	return 1;
}

void
eepromwrite(ushort addr, byte val)
{
	byte s;

	if(iomem.eecr & Eewe)
		sleep(&eeio.r, eedone, nil);	/* takes many milliseconds to write */
	s = splhi();
	iomem.eear[0] = addr;
	iomem.eear[1] = addr>>8;
	iomem.eedr = val;
	eestartw();
	splx(s);
}

byte
eepromread(ushort addr)
{
	if(iomem.eecr & Eewe)
		sleep(&eeio.r, eedone, nil);
	iomem.eear[0] = addr;
	iomem.eear[1] = addr>>8;
	iomem.eecr |= Eere;
	return iomem.eedr;
}

void
eeready(void*)
{
//	uartputc('!');
	iomem.eecr &= ~Eerie;
	wakeup(&eeio.r);
}
