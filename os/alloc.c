/*
 * allocator from Kernighan & Ritchie
 *
 * TO DO: is first fit better in this particular case?
 * could use gcalloc instead
 * it would be nice to have bounded interrupt lock-out time
 */

#include "u.h"
#include "mem.h"
#include "os.h"

typedef struct Header Header;
struct Header {
	Header*	next;
	uint	size;		/* could quantize to 8/16 byte blocks and use a single byte on the mote */
};
#define	NUNITS(n) (((n)+sizeof(Header)-1)/sizeof(Header) + 1)

enum {
	SLOP=	4*sizeof(Header),	/* don't split blocks smaller than this */
};

static	Lock	mlock;
static	Header	heaplist;
static	Header	*rover;
static	Header	checkval;
extern	char	end[];

void
mallocinit(void)
{
	uint nb;
	Header *p;

	nb = (char*)(SRAMZERO+SRAMSIZE) - end;
	nb -= MAINSTACK;	/* allow for main stack */
	p = (Header*)end;
	p->next = &checkval;
	p->size = nb/sizeof(Header);	/* truncate, don't round */
	heaplist.next = rover = &heaplist;
	free(p+1);
	print("mem=%d\n", nb);
}

void*
malloc(uint nbytes)
{
	Header *p, *q;
	uint nunits;

	nunits = NUNITS(nbytes);
	ilock(&mlock);
	q = rover;
	for(p = q->next;; q = p, p = p->next){
		if(p->size >= nunits){
			if(p->size > nunits+SLOP){
				p->size -= nunits;
				p += p->size;
				p->size = nunits;
			}else
				q->next = p->next;
			rover = q;
			p->next = &checkval;
			iunlock(&mlock);
			//memset(p+1, 0, nbytes);
			return p+1;
		}
		if(p == rover){	/* wrapped round */
			iunlock(&mlock);
			diag(Dnomem);
			print("malloc:mem\n");
			return nil;		/* or panic */
		}
	}
}

void
free(void *ap)
{
	Header *p, *q;

	if(ap == nil)
		return;
	p = (Header*)ap - 1;
	if(p->size == 0 || p->next != &checkval)
		panic("free:mem");
	ilock(&mlock);
	if((q = rover)==nil)
		panic("free:rover");
	for(; !(p > q && p < q->next); q = q->next)
		if(q >= q->next && (p > q || p < q->next))
			break;
	if(p+p->size == q->next){
		p->size += q->next->size;
		p->next = q->next->next;
	}else
		p->next = q->next;
	if(q+q->size == p){
		q->size += p->size;
		q->next = p->next;
	}else
		q->next = p;
	iunlock(&mlock);
	rover = q;
}

#ifdef DEBUG
void
mallocdump(void)
{
	Header *q;

	if((q = rover) != nil)
		do {
			print("%#p\t%d\n", q, q->size);
		} while((q = q->next) != rover);
}
#endif
