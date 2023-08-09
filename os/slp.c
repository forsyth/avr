#include "u.h"
#include "mem.h"
#include "os.h"

void
sleep(Rendez *r, byte (*f)(void*), void *arg)
{
	Proc *p;
	byte s;

	s = splhi();
	if(r->p)
		panic("sleep");
	if(f(arg)){
		splx(s);
		return;
	}
	p = curproc();
	r->p = p;
	p->state = Wakeme;
	sched();
	splx(s);
}

void
wakeup(Rendez *r)
{
	Proc *p;
	byte s;

	s = splhi();
	p = r->p;
	if(p != nil){
		if(p->state != Wakeme)
			panic("wakeup");
		r->p = nil;
		ready(p);
	}
	splx(s);
}
