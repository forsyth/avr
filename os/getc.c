#include "u.h"
#include "mem.h"
#include "os.h"


/*
 * character-oriented functions (unused)
 */
int
qgetc(Queue *q)
{
	Block *b;
	int c;

	for(c = -1; c < 0;){
		ilock(q);
		b = q->first;
		if(b == nil){
			q->state |= Qstarve;
			iunlock(q);
			return -1;
		}
		if(b->rp < b->wp){
			c = *b->rp++;
			q->len--;
		}
		if(b->rp >= b->wp){
			q->first = b->next;
			q->nb--;
			freeb(b);
		}
		if(DRAINED(q)){
			q->state &= ~Qflow;
			if(q->canput != nil)
				q->canput(q);
		}
		iunlock(q);
	}
	return c;
}

byte
qputc(Queue *q, byte c)
{
	Block *b;
	int starved;

	ilock(q);
	if(q->state & Qclosed){
		iunlock(q);
		return 0;
	}
	b = q->last;
	if(b == nil || b->wp >= b->lim){
		b = allocb(Blocksize);
		if(b == nil)
			return 0;
		if(q->first == nil)
			q->first = b;
		else
			q->last->next = b;
		q->last = b;
		q->nb++;
	}
	*b->wp++ = c;
	q->len++;
	if(q->len >= q->hiwat || q->nb >= Maxqblocks)
		q->state |= Qflow;
	starved = q->state & Qstarve;
	q->state &= ~Qstarve;
	iunlock(q);
	if(starved && q->canget != nil)
		q->canget(q);
	return 1;
}
