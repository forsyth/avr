#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"
#include "slip.h"

/*
 * rfc1055
 */

enum {
	End=	0300,
	Esc=	0333,
	Eend=	0334,	/* encoded End byte */
	Eesc=	0335,	/* encoded Esc byte */
};

ushort
slipenc(Block *b, ushort state)
{
	byte c;

	/* 0:End (1:(!End&!Esc) | (1:Esc 2:Eend) | (1:Esc 3:Eesc))* 0:End */
	switch(state>>8){
	case 0:
		return (1<<8) | End;
	case 1:
		if(b->rp < b->wp){
			c = *b->rp++;
			if(c == End)
				return (2<<8) | Esc;
			if(c == Esc)
				return (3<<8) | Esc;
			return (1<<8) | c;
		}
		return (0<<8) | End;
	case 2:
		return (1<<8) | Eend;
	case 3:
		return (1<<8) | Eesc;
	}
}

ushort
slipdec(byte c, ushort esc)
{
	if(esc == Slipesc){
		/* last byte was Esc */
		if(c == Eend)
			return End;
		if(c == Eesc)
			return Esc;
		return c;	/* as-is */
	}
	if(c == Esc)
		return Slipesc;
	if(c == End)
		return Slipend;
	return c;
}
