#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "adc.h"

#define DPRINT	if(0)print

static char hello[] = "davros\n";

static void
radioproc(void*)
{
	byte i;
	Block *b;
	ushort id, src, dst, hisid, t;
	byte db;

	id = shortid();
	setsinkhere(id);
	for(;;){
		macwaitrx();
		while((b = macread(&src, &dst)) != nil){
			if(0 && BLEN(b) == 2*(Dzed+1)){
				for(i = 0; i < Dzed+1; i++){
					print("%ud	", get2(b->rp));
					b->rp += 2;
				}
				print("\n");
				freeb(b);
				continue;
			}
			hisid = get2(b->rp);
			t = get2(b->rp+2);
			b->rp += 4;
			uartputi(0xfedc);
			uartputi(hisid);
			uartputi(t);
			// print("%ud	%ud", hisid, t);
			for(i = BLEN(b)-4; i > 0; i--){
				db = *b->rp++;
				uartputc(db);
				// print("	%ud", db);
			}
			// print("\n");
			freeb(b);
		}
	}
}

void
main(void)
{
	byte i;
	ushort id;

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

	/* i am the sink */
	id = shortid();
	setsinkhere(id);

	/* initialise radio */
	cc1kreset();
	radioinit();
	//cc1kdump();

	macinit(id, nil);
	spawn(radioproc, nil);

	DPRINT("sched\n");
	sched();
}
