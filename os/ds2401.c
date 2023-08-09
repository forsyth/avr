#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

/*
 * DS2401 serial number chip
 *
 * memory map: crc[1] serial[6] family[1]=01
 *	transmitted family first
 * crc is x⁸+x⁵+x⁴+1
 */
enum {
	A_serial_id_io=	1<<4,

	ReadROM=	0x33,
	IDsize=	1+6+1,
};

static byte
crc8(byte *p, byte n)
{
	byte i, v, crc;

	if(n == 0)
		return 0;
	crc = 0;
	do{
		v = *p++;
		for(i=0; i<8; i++){
			if((v^crc) & 1)
				crc = ((crc^0x18)>>1)|0x80;
			else
				crc >>= 1;
			v >>= 1;
		}
	}while(--n != 0);
	return crc;
}

void
ds2401init(void)
{
	iomem.ddra &= ~A_serial_id_io;
	iomem.porta &= ~A_serial_id_io;	/* tri-state */
}

static void
pinlow(void)
{
	iomem.ddra |= A_serial_id_io;	/* porta is set, giving output high as intermediate state */
	iomem.porta &= ~A_serial_id_io;	/* output low */
}

static void
pinrelease(void)
{
	iomem.porta |= A_serial_id_io;	/* output high as intermediate state */
	iomem.ddra &= ~A_serial_id_io;	/* input, and pull-up */
}

static void
dsout(byte b)
{
	byte i;

	for(i=0; i<8; i++){
		if(b & 1){
			pinlow();
			waitusec(1);	/* write 1 low time: 1-15 μs (slot is 60 μs) */
			pinrelease();
			waitusec(65);
		}else{
			pinlow();
			waitusec(65);	/* write 0 low time: 60-120 μs */
			pinrelease();
			waitusec(1);
		}
		b >>= 1;
	}
}

static byte
dsin(void)
{
	byte i, b;

	b = 0;
	for(i=0; i<8; i++){
		/* time slots: master drives line low 1-15 μs; slot is 60-120μs */
		b >>= 1;
		pinlow();
		waitusec(1);
		pinrelease();
		waitusec(10);
		if(iomem.pina & (byte)A_serial_id_io)
			b |= 0x80;
		waitusec(60);
	}
	return b;
}

byte
ds2401readid(byte *buf, byte bsize)
{
	byte i, n, s;

	if(bsize < IDsize)
		return 0;

	s = splhi();
	iomem.porta |= A_serial_id_io;	/* enable pull-up */
	/* reset by master: drive low, reset min 480μs */
	pinlow();
	waitusec(490);
	pinrelease();	/* release line, enter rx mode */

	/* presence detect high: 15-60μs */
	for(n=0; n<(70/10) && (iomem.pina & A_serial_id_io) != 0; n++)
		waitusec(10);	/* recovery time high */
	if(iomem.pina & (byte)A_serial_id_io)
		goto Err;
	/* read presence pulse from slave: duration 60-240μs */
	for(n=0; n<(240/10) && (iomem.pina & A_serial_id_io) == 0; n++)
		waitusec(10); /* duration 60-240μs */
	if(n >= (240/10))
		goto Err;

	dsout(ReadROM);
	for(i = 0; i < IDsize; i++)
		buf[i] = dsin();

	iomem.porta &= ~A_serial_id_io;
	splx(s);

	/* check CRC */
	if(crc8(buf, IDsize) != 0)
		return 0;

	return IDsize;

Err:
	iomem.porta &= ~A_serial_id_io;	/* tri-state */
	splx(s);
	return 0;
}
