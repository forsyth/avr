#include "u.h"
#include "mem.h"
#include "os.h"

/*
 * not needed in the usual case where scheduling provides mutual exclusion,
 * but might be needed to lock across wait-for-interrupt (for instance)
 */
void
qlock(QLock *q)
{
	Proc *p;

	if(q->lk == 0){
		q->lk = 1;
		return;
	}
	p = curproc();
	p->qnext = nil;
	if(q->head == nil)
		q->head = p;
	else
		q->tail->qnext = p;
	q->tail = p;
	p->state = Blocked;
	sched();
}

byte
canqlock(QLock *q)
{
	if(q->lk)
		return 0;
	q->lk = 1;
	return 1;
}

void
qunlock(QLock *q)
{
	Proc *p;

	p = q->head;
	if(p == nil){
		q->lk = 0;
		return;
	}
	/* hand over lock */
	q->head = p->qnext;
	ready(p);
}
