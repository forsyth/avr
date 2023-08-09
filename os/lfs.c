/*
 * little file system for serial data flash
 *	similar format to Matchbox at the moment,
 *	though this will change
 *
 *	no more than 15 files just now (one root page)
 */

#include "u.h"
#include "mem.h"
#include "os.h"

#define DPRINT	if(0)print

typedef struct Hdr Hdr;
typedef struct Meta Meta;
typedef struct Page Page;

enum {
	REAM=	0,	/* normal case */
//	REAM=	1,	/* once-for-all (ha ha) format of flash */

	Npages=	2048,
	Datasize=	256,
	Datashift=	8,	/* log2(Datasize) */
	Ctlsize=	8,	/* could steal from datasize if needed since there's no difference */
	Pagesize=	Datasize+Ctlsize,

	Namelen=	14,
	Entrylen=	Namelen+2,	/* name[14] page[2] */

	Nopage=	0xFFF,
	Pagemask=	0xFFF,

	/* tags */
	Tag=	7<<5,	/* mask */
	Troot=	1<<5,
	Tdata=	4<<5,
	Tfree=	7<<5,
};

/* stupid name: it's at the tail */
struct Hdr {
	byte	bits[3];	/* tag:3 lastbyte:9 nextpage:12 */
	byte	sum;
	byte	crc[2];
	byte	count[2];	/* really only need 14 (or even 13) bits for counter */
};

/* when unpacked */
struct Meta {
	byte	tag;
	ushort	used;
	ushort	next;
};

struct Page {
	/* representation on flash */
	byte	data[Datasize];
	Hdr;
};

static struct {
	byte	free[Npages/8];	/* initially ones */
	ushort	tfree;
	ushort	freep;
	ushort	root;
	ushort	vers;
} lfs;

static	void	scan(void);
static	void	ream(void);
static	ushort	findfree(void);

void
lfsinit(void)
{
	if(REAM)
		ream();
	/* find meta data */
	scan();
}

static ushort
allocpage(void)
{
	byte s, i, j, b;

	s = lfs.freep;
	i = s;
	do{
		lfs.freep = i;
		if(lfs.free[i] != 0){
			b = 1;
			for(j = 0; j < 8; j++, b <<= 1)
				if(lfs.free[i] & b){
					lfs.free[i] &= ~b;
					lfs.tfree--;
					return (i<<3) | j;
				}
		}
		if(++i >= nelem(lfs.free))
			i = 0;
	}while(i != s);
	return Nopage;
}

static void
freepage(ushort n)
{
	byte i, b;

	if(n == Nopage)
		return;
	if(n >= Npages)
		panic("freepg");
	i = n>>3;
	b = 1 << (n & 7);
	if(lfs.free[i] & b)
		panic("freepg2");
	lfs.free[i] |= b;
	lfs.tfree++;
}

static byte
isfree(ushort n)
{
	byte b;

	if(n >= Npages)
		return 0;
	b = 1<<(n&7);
	return lfs.free[n>>3] & b;
}

static byte
markused(ushort n)
{
	byte b, *m;

	if(n >= Npages)
		return 1;
	b = 1<<(n&7);
	m = &lfs.free[n>>3];
	if((*m & b) == 0)
		return 1;
	lfs.tfree--;
	*m &= ~b;
	return 0;
}

static byte
cksum(byte *b)
{
	byte x;

	x = b[0];
	x += b[1];
	x += b[2];
	return ~x;
}

static void
putpage(ushort n, Page *p)
{
	put2(p->crc, crc16block(p->data, Datasize));
	put2(p->count, get2(p->count)+1);
	if(!flashpagewrite(n, p, Pagesize, nil))
		DPRINT("lfs: page %d: flash write error\n", n);
}

static Page*
rootpage(void)
{
	Page *b;

	if(lfs.root == Nopage)
		return nil;
	b = malloc(sizeof(*b));
	if(b == nil)
		return nil;
	flashread(lfs.root, 0, b->data, Pagesize);
	if(cksum(b->bits) != b->sum ||
	  (b->bits[0] & Tag) != Troot ||
	   crc16block(b->data, Datasize) != get2(b->crc)){
		DPRINT("lfs: page %d: bad root page\n", lfs.root);
		return nil;
	}
	return b;
}

static void
newroot(Page *p)
{
	ushort n;

	n = allocpage();
	put2(p->data, lfs.vers+1);
	put2(p->crc, crc16block(p->data, Datasize));
	put2(p->count, get2(p->count)+1);
	if(flashpagewrite(n, p, Pagesize, nil)){
		freepage(lfs.root);
		lfs.root = n;
		lfs.vers++;
	}else{
		freepage(n);
		DPRINT("lfs: page %d: flash write error\n", n);
	}
}

static void
mkhdr(Hdr *h, byte tag, ushort nb, ushort next)
{
	/* tag:3 lastbyte:9 nextpage:12 */
	h->bits[0] = tag | (nb>>4);
	h->bits[1] = (nb<<4) | (next>>8);
	h->bits[2] = next;
	h->sum = cksum(h->bits);
}

static void
setnext(Hdr *h, ushort next)
{
	h->bits[1] &= 0xF0;
	h->bits[1] |= next>>8;
	h->bits[2] = next;
	h->sum = cksum(h->bits);
}

static byte
readmeta(ushort p, Meta *m)
{
	static Hdr h;

	flashread(p, Datasize, &h, sizeof(h));
	if(cksum(h.bits) != h.sum || (h.bits[0] & Tag) != Tdata)
		return 0;
	m->tag = h.bits[0] & Tag;
	m->next = ((h.bits[1]<<8) | h.bits[2]) & Pagemask;
	m->used = (h.bits[0] & ~Tag)<<4;
	m->used |=  h.bits[1]>>4;
	return 1;
}

static void
ream(void)
{
	ushort p;
	Page *b;

	DPRINT("lfs: ream %d pages\n", Npages);

	/* initialise all but root page */
	b = malloc(sizeof(*b));
if(0){
	memset(b, 0xFF, Pagesize);
	put2(b->count, 0);	/* should do this only once per flash */
	for(p = 1; p < Npages; p++)
		flashpagewrite(p, b, Pagesize, nil);
}

	/* write root page */
	memset(lfs.free, 0xFF, sizeof(lfs.free));	/* all free */
	lfs.tfree = Npages;
	p = allocpage();
	lfs.root = p;
	lfs.vers = 0;
	memset(b->data, 0, Datasize);	/* sets version to zero and clears all entries */
	put2(b->count, 0);	/* should really be done by separate format operation */
	mkhdr(b, Troot, 2, Nopage);
	putpage(0, b);
	free(b);
}

static void
scan(void)
{
	ushort p, vers, np, t;
	Page *b;
	Meta m;
	char *name;

	DPRINT("lfs scan\n");
	lfs.root = Nopage;
	lfs.vers = 0;
	memset(lfs.free, 0xFF, sizeof(lfs.free));	/* all free */
	lfs.tfree = Npages;
	b = malloc(sizeof(*b));

	/* look for the most recent root page */
	for(p=0; p<Npages; p++){
		flashread(p|Unbuffered, Datasize, &b->Hdr, sizeof(Hdr));
		if(cksum(b->bits) == b->sum &&
		   (b->bits[0] & Tag) == Troot){
			flashread(p, 0, b->data, Datasize);
			if(crc16block(b->data, Datasize) != get2(b->crc)){
				DPRINT("lfs: page %d: bad root crc\n", p);
				continue;
			}
			vers = get2(b->data);
			DPRINT("lfs: page %d: root %d\n", p, vers);
			if(lfs.root == Nopage || lfs.vers < vers){
				lfs.root = p;
				lfs.vers = vers;
			}
		}
		/* otherwise ignore it during this pass */
	}

	DPRINT("lfs: use root page %d vers %d\n", lfs.root, lfs.vers);

	if(lfs.root == Nopage){
		free(b);
		return;
	}

	DPRINT("lfs: trace\n");
	/* trace file system through root page */
	markused(lfs.root);
	lfs.freep = lfs.root;	/* it's assumed to be fairly recently rewritten */
	for(t = Entrylen; t < Datasize; t += Entrylen){
		name = (char*)b->data+t;
		if(*name == 0)
			continue;	/* unused */
		DPRINT("%s\n", name);
		for(p = get2((byte*)name+Namelen); p != Nopage; p = np)
			if(readmeta(p|Unbuffered, &m)){
				if(markused(p)){
					DPRINT("%s: dup %d\n", name, p);
					break;
				}
				DPRINT("  %d", p);
				np = m.next;
			}else{
				DPRINT("%s: %d bad hdr\n", name, p);
				break;
			}
		DPRINT("\n");
	}
	DPRINT("lfs: trace end\n");
	free(b);
	{extern int fltrace; fltrace = 1;}
}

File*
fopen(char *name)
{
	File *f;
	Page *r;
	char *en;
	ushort p;
	ushort t;

	if(lfs.root == Nopage)
		return nil;
	r = rootpage();
	if(r == nil)
		return nil;
	for(t = Entrylen; t < Datasize; t += Entrylen){
		en = (char*)r->data + t;
		if(strncmp(en, name, Namelen) != 0)
			continue;
		p = get2((byte*)en+Namelen);
		free(r);
DPRINT("%s: dslot %d page %d\n", name, t, p);
		f = malloc(sizeof(*f));
		if(f == nil)
			return nil;
		memset(f, 0, sizeof(*f));
		f->name = name;
		f->dirslot = t;
		f->initial = p;
		f->here = ~0;
		fsetpos(f, 0);
		return f;
	}
	free(r);
	return nil;
}

File*
fcreate(char *name)
{
	File *f;
	Page *r;
	char *en;
	ushort p;
	ushort t, e;

	if(lfs.root == Nopage){
		ream();
		if(lfs.root == Nopage){
			panic("ream");
			return nil;
		}
	}
	if(lfs.tfree < 2){
		/* need new root page (but we're freeing the old one?) and first data page */
	}
	r = rootpage();
	if(r == nil)
		return nil;
	p = Nopage;
	e = 0;
	for(t = Entrylen; t < Datasize; t += Entrylen){
		en = (char*)r->data + t;
		if(*en == 0){
			if(e == 0)
				e = t;
		}else if(strncmp(en, name, Namelen) == 0){
			p = get2((byte*)en+Namelen);
			break;
		}
	}
DPRINT("t=%d e=%d\n", t, e);
	if(t == Datasize){
		/* make new entry */
		if(e == 0)
			return nil;	/* no room */
		en = (char*)r->data + e;
		strncpy(en, name, Namelen);
		p = Nopage;
		put2((byte*)en+Namelen, p);
		t = e;
		/* for now we'll overwrite it; see newroot though */
		put2(r->data, ++lfs.vers);
		putpage(lfs.root, r);	/* allocate first page to file first? */
	}
	free(r);
	f = malloc(sizeof(*f));
	if(f == nil)
		return nil;
	memset(f, 0, sizeof(*f));
	f->name = name;
	f->dirslot = t;
	f->initial = p;
	f->here = ~0;
	fsetpos(f, 0);
	return f;
}

byte
fsetpos(File *f, ushort pos)
{
	Meta m;

	if(pos < f->here){
		f->here = 0;
		f->page = f->initial;
		if(f->page == Nopage){
			f->next = Nopage;
			f->bytes = 0;
		}else if(readmeta(f->page|Unbuffered, &m)){
			f->next = m.next;
			f->bytes = m.used;
		}else{
			DPRINT("%s: %d bad page (l%d)\n", f->name, f->page, f->here);
			f->page = Nopage;
			return 0;
		}
	}
	for(; f->here < pos && f->next != Nopage; f->here++){
		if(!readmeta(f->next|Unbuffered, &m)){
			DPRINT("%s: %d bad hdr (l%d)\n", f->name, f->next, f->here);
			return 0;
		}
		if(isfree(m.next)){
			DPRINT("%s: %d not alloc (l%d)\n", f->name, m.next, f->here);
			return 0;
		}
		f->page = f->next;
		f->bytes = m.used;
		f->next = m.next;
	}
	return f->here == pos;
}

void
fchop(File *f)
{
	ushort p;
	Meta m;
	Page *r;

	/* truncate file at current point */
	if(f->page == Nopage)
		return;
	/* clear the pointer */
	if(f->page == f->initial){	/* update root page; eventually use newroot */
		r = rootpage();
		put2(r->data+f->dirslot+Namelen, Nopage);
		put2(r->data, ++lfs.vers);
		putpage(lfs.root, r);
	}else{
		r = malloc(sizeof(*r));
		flashread(f->page, 0, r, Pagesize);
		setnext(&r->Hdr, Nopage);
		if(!flashpagewrite(f->page, r, Pagesize, nil))
			panic("fchop");
		free(r);
	}
	/* free following pages; any point in erasing them? */
	p = f->next;
	f->next = Nopage;
	while(p != Nopage){
		if(!readmeta(p|Unbuffered, &m))
			break;
		freepage(p);
		p = m.next;
	}
}

void
remove(char *name)
{
	ushort p;
	Meta m;
	Page *r;
	File *f;

	f = fopen(name);
	if(f == nil)
		return;
	r = rootpage();
	r->data[f->dirslot] = 0;
	put2(r->data, ++lfs.vers);
	putpage(lfs.root, r);
	/* free following pages; any point in erasing them? */
	for(p = f->initial; p != Nopage;){
		if(!readmeta(p|Unbuffered, &m))
			break;
		freepage(p);
		p = m.next;
	}
	fclose(f);
}

char
fwrite(File *f, void *a, int n)
{
	ushort p;
	Hdr h;
	Page *r;

	if(n <= 0)
		return 0;
	if(n > Datasize)
		return 0;	/* temporary restriction */
	/* must write the data page before updating the reference to it */
	p = allocpage();
	if(p == Nopage)
		return 0;
	mkhdr(&h, Tdata, n, Nopage);
	put2(h.crc, crc16block(a, n));
	put2(h.count, get2(h.count)+1);
	if(!flashpagewrite(p, a, n, (byte*)&h))
		goto Err;
	if(f->initial == Nopage){
		r = rootpage();
		if(r == nil)
			goto Err;
		if(get2(r->data+f->dirslot+Namelen) != Nopage)
			panic("fwrite");
		put2(r->data+f->dirslot+Namelen, p);
		put2(r->data, ++lfs.vers);
		putpage(lfs.root, r);	/* allocate first page to file first? */
		free(r);
	}else{
		/* add to end of file */
		fsetpos(f, ~0);
		if(f->page == Nopage)
			panic("fwrite");
		/* TO DO: read to buffer, update buffer, write buffer? */
		/* flashpageupdate(hdr) */
		r = malloc(sizeof(*r));
		flashread(f->page|Unbuffered, 0, r->data, Pagesize);
		setnext(&r->Hdr, p);
		if(!flashpagewrite(f->page, r, Pagesize, nil)){
			free(r);
			goto Err;
		}
		free(r);
		f->next = p;
	}
	return 1;

Err:
	freepage(p);
	return 0;
}

int
fread(File *f, void *a, int n, long o)
{
	ushort b;

	if(n <= 0)
		return 0;
	if(!fsetpos(f, o>>Datashift) || f->page == Nopage)
		return 0;
	b = o & (Datasize-1);
	if(b >= f->bytes)
		return 0;
	if(b+n > f->bytes)
		n = f->bytes - b;
	flashread(f->page, b, a, n);
	return n;
}

void
fclose(File *f)
{
	/* perhaps nothing else to do here */
	free(f);
}
