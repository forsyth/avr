#include "u.h"
#include "mem.h"
#include "os.h"

enum {
	Maxqblocks=	4,	/* absolute limit to queue size (hiwat might be less) */
};

void
blockinit(void)
{
}

Block*
allocb(int n)
{
	Block *b;

	if(n < 0 || n > Blocksize)
		panic("allocb");
	b = malloc(sizeof(Block));
	if(b == nil){
		diag(Dnoblk);
		return nil;
	}
	b->next = nil;
	b->lim = b->base + Blocksize;
	b->rp = b->wp = b->lim - n;
	b->flag = 0;
	b->ctl = 0;
	b->magic = 0xb00f;
	b->ticks = 0;
// print("A%p|", b);
	return b;
}

void
freeb(Block *b)
{
	if(b != nil && (b->flag & Bowned) == 0){
// print("F%p|", b);
		if(b->magic != 0xb00f){
// print("%d %d %d %d\n", b->magic, curproc()->stack, stkpc(), stkfp());
			panic("freeb");
		}
		b->magic = 0;
		free(b);
	}
}

void
freeblist(Block *b)
{
	Block *nb;

	for(; b != nil; b = nb){
		nb = b->next;
		freeb(b);
	}
}

Block*
bprefix(Block *b, int n)
{
	if(n < 0 || b->rp - b->base < n)
		panic("bprefix");
	b->rp -= n;
	return b;
}

short
bticks(Block *b)
{
	int dt;
	ushort t;
	byte s;

	s = splhi();
	t = fastticks;
	splx(s);
	dt = t - b->ticks;
	if(dt < 0)
		dt = -dt;
	return dt;
}

#define	DRAINED(q) ((q)->state & Qflow && (q)->nb <= Maxqblocks/2)

void
queueinit(void)
{
}

/*
 * allocate and initialise a single queue
 */
Queue*
qalloc(Qproto *qp)
{
	Queue *q;

	q = malloc(sizeof(*q));
	if(q == nil)
		panic("qalloc");
	memset(q, 0, sizeof(*q));
	q->hiwat = qp->hiwat;
	if(q->hiwat == 0 || q->hiwat > Maxqblocks)
		q->hiwat = Maxqblocks;
	q->nb = 0;
	q->put = qp->put;
	q->get = qp->get;
	q->ctl = qp->ctl;
	q->canput = qp->canput;
	q->canget = qp->canget;
	return q;
}

void
qfree(Queue *q)
{
	if(q == nil)
		return;
	qclose(q);
	q->state = 0;
	q->canput = nil;
	q->canget = nil;
	q->ctl = nil;
	q->put = nil;
	free(q);
}

/*
 * flow control
 */

byte
qcanget(void *a)
{
	return ((Queue*)a)->first != nil || ((Queue*)a)->state & Qclosed;
}

Block *
qget(Queue *q)
{
	Block *b;

	if(q->get != nil)
		return q->get(q);
	ilock(q);
	if((b = q->first) != nil){
		q->first = b->next;
		b->next = nil;
		q->nb--;
		if(DRAINED(q)){
			q->state &= ~Qflow;
			if(q->canput != nil)
				q->canput(q);
		}
	}else if(q->next == nil || (q->next->state & Qflow) == 0)
		q->state |= Qstarve;
	iunlock(q);
	return b;
}

byte
qcanput(void *a)
{
	return (((Queue*)a)->state & (Qflow|Qclosed)) == 0;
}

void
qput(Queue *q, Block *b)
{
	int n, starved;
	Block *ob, *np;

	ilock(q);
	for (np = b; (b = np) != nil;) {
		np = b->next;
		b->next = nil;
		n = BLEN(b);
		if(n == 0 && q->state & Qcoalesce) {
			freeb(b);
			continue;
		}
		q->nb++;
		if(q->first != nil) {
			ob = q->last;
			if(q->state & Qcoalesce &&
			    b->ctl == 0 && ob->ctl == 0 &&
			    ob->lim - ob->wp >= n) {
				q->nb--;
				memmove(ob->wp, b->rp, n);
				ob->wp += n;
				freeb(b);
			} else
				q->last = ob->next = b;
		}else
			q->first = q->last = b;
		if(q->nb >= q->hiwat)
			q->state |= Qflow;
	}
	starved = q->state & Qstarve;
	q->state &= ~Qstarve;
	iunlock(q);
	if(starved && q->canget != nil)
		q->canget(q);
}

byte
qblen(Queue *q)
{
	return q->nb;
}

/*
 * remove all data from a queue
 */
void
qflush(Queue *q)
{
	Block *b;

	ilock(q);
	b = q->first;
	q->first = nil;
	q->nb = 0;
	iunlock(q);
	freeblist(b);
	ilock(q);
	if(DRAINED(q)){
		q->state &= ~Qflow;
		if(q->canput != nil)
			q->canput(q);
	}
	iunlock(q);
}

void
qhangup(Queue *q)
{
	ilock(q);
	q->state |= Qclosed;
	if(q->state & Qstarve && q->nb && q->canget != nil)
		q->canget(q);
	if(q->state & Qflow && q->canput != nil)
		q->canput(q);
	iunlock(q);
}

void
qclose(Queue *q)
{
	Block *bl;

	if(q == nil)
		return;
	ilock(q);
	q->state |= Qclosed;
	q->state &= ~(Qflow|Qstarve);
	bl = q->first;
	q->first = nil;
	q->nb = 0;
	iunlock(q);
	freeblist(bl);
	ilock(q);
	if(DRAINED(q) && q->canput != nil)
		q->canput(q);
	iunlock(q);
}

void
qreopen(Queue *q)
{
	ilock(q);
	q->state &= ~Qclosed;
	q->state |= Qstarve;
	iunlock(q);
}

/*
 * Put the given list of blocks at the head of a queue.
 * Usually called to return data that can't yet be processed.
 */
void
qputback(Queue *q, Block *b)
{
	int nb;
	Block *lp;

	for(lp = b, nb = 1; lp->next != 0; lp = lp->next)
		nb++;
	ilock(q);
	if ((lp->next = q->first) == nil)
		q->last = lp;
	q->first = b;
	q->nb += nb;
	if(q->nb >= q->hiwat)
		q->state |= Qflow;
	iunlock(q);
}
