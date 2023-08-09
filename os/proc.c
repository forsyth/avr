#include "u.h"
#include "mem.h"
#include "os.h"

static struct {
	Lock;
	Proc*	p;	/* current */
	Task*	h;	/* ready list */
	Task*	t;
	Proc*	f;	/* list of Procs waiting to be freed */
} tasklist;

static void
_tramp(int, Task *p)
{
//print("tramp: %4.4ux\n", (int)&p);
	if(p == nil)
		panic("tramp");
	spllo();
	p->f(p->arg);
	pexit();
}

void
procinit(void)
{
}

Proc*
spawn(void (*f)(void*), void *arg)
{
	Proc *p;

	p = tasklist.f;	/* see if one waiting to be freed (can't happen) */
	if(p == nil){
		p = malloc(sizeof(Proc));
		if(p == nil)
			panic("noprocs");
	}else
		tasklist.f = (Proc*)p->sched;
	p->sched = nil;
	p->pc = (int)_tramp;
	p->sp = (int)p->stack+sizeof(p->stack);
	p->fp = p->sp - HWSTACK - 2*BY2WD;	/* 2 arguments */
	((ushort*)p->fp)[1] = (ushort)p;	/* second argument */
//print("p->fp=%.4ux\n", p->fp);
	p->f = f;
	p->arg = arg;
	p->state = Blocked;
	ready(p);
	return p;
}

static void
schedule(Task *p)
{
	p->sched = nil;
	if(tasklist.h == nil)
		tasklist.h = p;
	else
		tasklist.t->sched = p;
	tasklist.t = p;
}

void
ready(Proc *p)
{
//print("ready: %4.4ux\n", (int)p);
	ilock(&tasklist);
	if(p->state == Inactive || p->state == Transient)
		panic("ready");
	p->state = Ready;
	schedule(p);
	iunlock(&tasklist);
}

void
pexit(void)
{
	Task *p;

	p = tasklist.p;
	tasklist.p = nil;
	if(p != nil){
		if(p->state == Transient)
			panic("pexit");
		p->state = Inactive;
		p->sched = tasklist.f;
		tasklist.f = (Proc*)p;
	}
	sched();
}

void
transient(Task *p, void (*f)(void*), void *arg)
{
	p->sched = nil;
	p->state = Transient;
	p->f = f;
	p->arg = arg;
}

void
activate(Task *p)
{
	ilock(&tasklist);
	if(p->state == Transient){
		p->state = Pending;
		schedule(p);
	}else if(p->state != Pending)
		panic("activate");
	iunlock(&tasklist);
}

void
sched(void)
{
	Task *t;
	Proc *p;
	byte s;

//print("sched\n");
	s = splhi();
	p = tasklist.p;
	if(p != nil){
		if(!((byte*)&p > p->stack && (byte*)&p < p->stack+sizeof(p->stack)))
			panic("stack");
		tasklist.p = nil;
		if(p->state == Ready)
			ready(p);
		if(setlabel(p)){
			splx(s);
			/* cannot now be running on exiting Proc's stack: can free */
			while((p = tasklist.f) != nil){	/* `if' would do */
				tasklist.f = (Proc*)p->sched;
				free(p);
			}
			return;
		}
	}
	for(;;){
		if((t = tasklist.h) != nil){
			tasklist.h = t->sched;
			if(t->state != Pending){
				p = (Proc*)t;
				tasklist.p = p;
				gotolabel(p);	/* no return */
			}
			t->state = Transient;
			spllo();
			t->f(t->arg);
			splhi();
		}else
			idlehands();
	}
}

Proc*
curproc(void)
{
	Proc *p;

	p = tasklist.p;
	if(p == nil || p->state == Transient)
		panic("curproc");
	return p;
}

byte
return0(void*)
{
	return 0;
}
