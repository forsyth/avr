#include "u.h"
#include "mem.h"
#include "os.h"

struct Timer {
	unsigned long	time;
	void	(*f)(void*, int);
	void	*arg;
	Timer	**ref;
};

enum {
	NTimer=	5
};

static	Timer	callout[NTimer];
static	Timer	*callp[NTimer];
static	int	nextcall;
static	int	delay;

static Timer *delete(Timer **);
static void pushup(Timer **);
static void pushdown(Timer **);

/* store a reference to Timer b in a, maintaining back pointer */
#define	setref(a, b) (*(a) = (b), (*(a))->ref = (a))

/*
 * Register f(arg) to be called by timein
 * shortly after delta clock ticks have elapsed.
 */
Timer *
timeout(void (*f)(void*, int), void *arg, long delta)
{
	Timer *cp, **del, **r = callp;
	int s = splhi();

	if(delay)
		pushup(&r[nextcall++]);
	if(nextcall >= NTimer)
		panic("timeout");
	del = &r[nextcall];	/* delay slot */
	cp = *del;
	cp->f = f;
	cp->arg = arg;
	cp->time = clockticks + delta;
	cp->ref = del;
	if(nextcall > 0 && cp->time < r[0]->time){
		/* exchange new entry and root */
		setref(del, r[0]);
		setref(&r[0], cp);
	}
	delay = 1;
	splx(s);
	return cp;
}

void
stoptimer(void (*f)(void*, int), void *arg, Timer *cp)
{
	if(cp != nil){
		int s = splhi();
		if(cp->ref != nil && cp->f == f && cp->arg == arg)
			delete(cp->ref);
		splx(s);
	}
}

/*
 * Has a timeout expired?
 */
int
timercheck(void)
{
	return callp[0]->time <= clockticks && (nextcall > 0 || delay);
}

/*
 * Timein is called by the machine-dependent interval clock handler
 * when timercheck() shows a timeout ready to run, no other interrupt
 * routines are running, and no other instance of timein is running.
 * Each expired entry is run in turn.
 */
void
timein(void)
{
	Timer *cp, **r = callp;
	int s, late;
	void *arg;
	void (*f)(void*, int);

	for(;;){
		s = splhi();
		if(r[0]->time > clockticks ||
		    (cp = delete(r)) == nil){
			splx(s);
			break;
		}
		/* must copy since timeout can be called by higher-priority interrupts */
		f = cp->f;
		arg = cp->arg;
		late = cp->time - clockticks;
		splx(s);
		(*f)(arg, late);
	}
}

/*
 * functions to access a heap:
 * p is a parent pointer, c is a child
 */
#define	parent(c,heap) ((heap) + ((unsigned)((c)-(heap+1))>>1))
#define child1(p,heap) ((heap+1) + (((p)-(heap))<<1))

static Timer *
delete(Timer **e)
{
	Timer **r = callp, **exch, *cp;

	if(e == nil || nextcall <= 0 && !delay)
		return nil;
	cp = *e;
	if(delay){
		/* exchange e and del */
		exch = &r[nextcall];
		delay = 0;
	}else
		exch = &r[--nextcall];	/* exchange e and last */
	if(e != exch){ /* neither delayed or last entry */
		setref(e, *exch);
		setref(exch, cp);
		if(e > r && parent(e,r)[0]->time > e[0]->time){
			pushup(e);
			cp->ref = nil;
			return cp;
		}
		pushdown(e);
	}
	cp->ref = nil;
	return cp;
}

static void
pushup(Timer **c)
{
	Timer **r = callp, **p, *v;

	for(v = *c; c > r && (p = parent(c,r))[0]->time > v->time; c = p)
		setref(c, *p);
	setref(c, v);
}

static void
pushdown(Timer **p)
{
	Timer **r = callp, **c, **e, *v;

	e = &r[nextcall-1];
	for(v = *p; (c = child1(p,r)) <= e; p = c){
		if(c < e && c[0]->time > c[1]->time)
			c++;
		if(c[0]->time >= v->time)
			break;
		setref(p, *c);
	}
	setref(p, v);
}

void
timerinit(void)
{
	int i;

	nextcall = 0;	/* callp[nextcall] is the delay slot */
	for(i = 0; i < NTimer; i++)
		callp[i] = &callout[i];
}
