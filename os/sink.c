#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

/*
 * sink routing
 */

/*
 * SINK[src, sink-addr, hops]
 *	- broadcast
 *	- choose next-hop with fewer hops
 *	- if next-hop times out (no longer mate), set to Noroute
 *	- change of sink-addr?
 *
 * NODE[src, node-addr, n, route[n-1], ... route[0]]
 *	- unicast along reverse-route (find current address in list and send to next below)
 *	- long random delay before sending
 *
 * in practice, we include the SINK data in the MAC sync, since that's already broadcast
 */

typedef struct Route Route;
struct Route {
	ushort	dst;
	ushort	hops;
};

static Route sink = {Noroute, Noroute};

void
setsinkhere(ushort a)
{
	sink.dst = a;
	sink.hops = 0;
	// print("route %d %d\n", a, 0);
}

byte
issink(void)
{
	return sink.hops == 0;
}

void
newsink(Block *b)
{
	ushort dst, hops;

	if(b->rp+4 > b->wp)
		return;
	dst = (b->rp[1]<<8) | b->rp[0];
	hops = (b->rp[3]<<8) | b->rp[2];
	if(hops < sink.hops){
		sink.dst = dst;
		sink.hops = hops;
		// print("route %d %d\n", dst, hops);
	}
}

void
noroutemate(ushort dst)
{
	if(sink.dst == dst){
		sink.hops = Noroute;
		sink.dst = Noroute;
		// print("noroute %d %d\n", Noroute, Noroute);
	}
}

ushort
sinkroute(void)
{
	return sink.dst;
}

void
addsink(Block *b, ushort iam)
{
	ushort v;

	if(b->wp+4 > b->lim)
		return;
	b->wp[0] = iam;	/* get to sink.dst through us */
	b->wp[1] = iam>>8;
	v = sink.hops;
	if(v != Noroute)
		v++;		/* cost of further hop through us */
	b->wp[2] = v;
	b->wp[3] = v>>8;
	b->wp += 4;
}

/*
 * find local address in route list,
 * and send to the address before it in the list.
 */
byte
routenext(Block *b, byte o, ushort iam)
{
	byte i, n;
	byte *p;
	ushort a, oa;

	p = b->rp+o;
	if(p > b->wp)
		return Noroute;
	n = *p++;
	if(n == 0)
		return 0;
	oa = 0;
	for(i=0; i < n; i++){
		a = (p[1]<<8) | p[0];
		if(a == iam)
			return oa;
		oa = a;
	}
	return Noroute;
}
