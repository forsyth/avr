#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "adc.h"

#define DPRINT	if(0)print

static char hello[] = "davros\n";

void
main(void)
{
	byte i;

	portinit();
	uart0init();
	mallocinit();
	// timerinit();
	clockinit();
	blockinit();
	queueinit();
	ds2401init();
	procinit();
	adcinit();
	flashinit();

	/* sanity test */
	i = Red|Yellow|Green;
	if(hello[0] != 'd')
		i &= ~Red;
	ledset(i);
	uartputs(hello, -1);

	moteid();

	/* initialise radio */
	cc1kreset();
	radioinit();
	//cc1kdump();

	macinit(shortid(), nil);

	spawn(fftproc, "fft");

	DPRINT("sched\n");
	sched();
}
