#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

enum {
	/* reset status (for reference, not used yet) */
	Jtrf=		1<<4,	/* JTAG reset */
	Wdrf=	1<<3,	/* watchdog reset */
	Borf=	1<<2,	/* brown-out reset */
	Extrf=	1<<1,	/* external reset */
	Porf=	1<<0,	/* power-on reset */

	/* watchdog */
	Wdce=	1<<4,	/* watchdog change enable */
	Wde=	1<<3,	/* watchdog enable */
	/* low-order three bits controls prescale: 2^(n+4) wdt oscillator cycles */
};

void
watchdogset(ushort ms)
{
	byte i;

	ms = (ms+15)>>4;
	for(i=0; i<7 && ms > (1<<i); i++){
		/* skip */
	}
	iomem.wdtcr = Wde | Wdce | i;
	iomem.wdtcr = Wde | i;
}

void
watchdogoff(void)
{
	/* works only if allowed by safety level */
	iomem.wdtcr = Wde | Wdce;
	iomem.wdtcr = 0;
}
