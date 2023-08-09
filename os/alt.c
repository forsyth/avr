#include "u.h"
#include "mem.h"
#include "os.h"

static char Ebusy[] = "busychan";
static char Ealtdone[] = "altdone";

static void altdone(Proc*, Channel*, int*);

byte
alt(Alt *alts)
{
	byte op;
	Alt *a;
	Proc *p, **q;
	Channel *ac;

	/* check ready alternatives */
	for(a = alts; (op = a->op) != CHANEND; a++){
		if(op == CHANNOBLK)
			return a-alts;
		ac = a->c;
		if(op == CHANNOP || ac == nil)
			continue;
		if(op == CHANRCV && (p = ac->s) != nil ||
		   op == CHANSND && (p = ac->r) != nil){
			altdone(p, ac, a->v);
			ready(p);
			if(op == CHANSND)
				sched();	/* let it use the value */
			return a-alts;
		}
	}

	/* queue for all */
	p = curproc();
	for(a = alts; a->op != CHANEND; a++){
		op = a->op;
		ac = a->c;
		if(op == CHANNOP || ac == nil)
			continue;
		if(op == CHANSND)
			q = &ac->s;
		else
			q = &ac->r;
		if(*q != nil)
			panic(Ebusy);
		*q = p;
	}

	/* block; sender/receiver will restart us */
	p->ptr = alts;
	p->state = Altq;
	sched();
	return (Alt*)p->ptr - alts;
}

static void
altdone(Proc *p, Channel *c, int *v)
{
	Alt *a;
	byte op;
	Channel *ac;

	if(p->state == Sendq){
		*v = *(int*)p->ptr;
		c->s = nil;
		return;
	}
	if(p->state == Recvq){
		*(int*)p->ptr = *v;
		c->r = nil;
		return;
	}
	if(p->state != Altq)
		panic(Ealtdone);
	/* take alt out of other comms */
	for(a = p->ptr; a->op != CHANEND; a++){
		op = a->op;
		if(op == CHANNOP)
			continue;
		ac = a->c;
		if(ac == c)
			p->ptr = a;	/* mark choice */
		if(0)
			print("proc %p: %p op %s c %p v %p [%d]\n", curproc(), p, op==CHANSND?"s":"r", c, v, *v);
		if(op == CHANSND){
			if(ac == c)
				*v = *(int*)a->v;
			ac->s = nil;
		}else if(op == CHANRCV){
			if(ac == c)
				*(int*)a->v = *v;
			ac->r = nil;
		}else
			panic(Ealtdone);
	}
}

void
send(Channel *c, int v)
{
	Proc *p;

	if((p = c->r) == nil){
		if(c->s != nil)
			panic(Ebusy);
		p = curproc();
		c->s = p;
		p->state = Sendq;
		p->ptr = &v;
		sched();
		return;
	}
	altdone(p, c, &v);
	ready(p);
	sched();	/* let it use the value */
}

int
recv(Channel *c)
{
	Proc *p;
	int v;

	if((p = c->s) == nil){
		if(c->r != nil)
			panic(Ebusy);
		p = curproc();
		c->r = p;
		p->state = Recvq;
		p->ptr = &v;
		sched();
		return v;
	}
	altdone(p, c, &v);
	ready(p);
	return v;
}

void
sendp(Channel *c, void *v)
{
	send(c, (int)v);
}

void*
recvp(Channel *c)
{
	return (void*)recv(c);
}

byte
nbsend(Channel *c, int v)
{
	Proc *p;

	if((p = c->r) == nil)
		return 0;
	altdone(p, c, &v);
	ready(p);
	sched();	/* let it use the value */
	return 1;
}

byte
nbrecv(Channel *c, int *v)
{
	Proc *p;

	if((p = c->s) == nil)
		return 0;
	altdone(p, c, v);
	ready(p);
	return 1;
}

byte
nbsendp(Channel *c, void *v)
{
	return nbsend(c, (int)v);
}

void*
nbrecvp(Channel *c)
{
	void *v;

	if(nbrecv(c, (int*)&v))
		return v;
	return nil;
}
