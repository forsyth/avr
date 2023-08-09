#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

/*
 * i2c master mode using TWI interface
 */

enum {
	/* twcr */
	Twint=	1<<7,	/* TWI interrupt ready; clear by writing 1 */
	Twea=	1<<6,	/* TWI enable acknowledge (of slave address) */
	Twsta=	1<<5,	/* start condition; cleared by software */
	Twsto=	1<<4,	/* stop condition; cleared automatically */
	Twwc=	1<<3,	/* write collision error (attempted write to TWDR with Twint low) */
	Twen=	1<<2,	/* enable TWI subsystem */
	Twie=	1<<0,	/* interrupt enable */

	/* twsr */
	Tws=	0xF8,	/* bus status */
	  Tstart=	0x08,	/* start condition transmitted */
	  Trstart=	0x10,	/* repeated start transmitted */
	  ArbLost=	0x38,	/* lost arbitration in address or data */
					/* master transmitter states: */
	  WslaAck=	0x18,	/* slave address+W transmitted; acked */
	  WslaNak= 0x20,	/* slave address+W transmitted; nacked */
	  WdataAck=	0x28,	/* data byte transmitted; acked */
	  WdataNak=	0x30,	/* data byte transmitted; nacked */
	  				/* master receiver states: */
	  RslaAck=	0x40,	/* slave address+R transmitted; acked */
	  RslaNak=	0x48,	/* slave address+R transmitted; nacked */
	  RdataAck=	0x50,	/* data byte received; acked */
	  RdataNak=	0x58,	/* data byte received; nacked */
	Twps1=	0<<0,	/* prescaler 1 */
	Twps4=	1<<0,	/* prescaler 4 */
	Twps16=	2<<0,	/* prescaler 16 */
	Twps64=	3<<0,	/* prescaler 64 */

	/* slave address register */
	Twgce=	1<<0,	/* general call recognition enable */

	Freq=	75000L,	/* try 75kHz initially */
	Wbit=	0<<0,	/* write mode in address byte */
	Rbit=	1<<0,	/* read mode in address byte */

	Trace=	0,
};

static struct {
	/* would have a QLock if there were more than one device on i2c */
	Lock;
	byte	state;
	byte	rw;	/* Read or Write */
	byte	slave;	/* slave address, including mode bit */
	byte*	d;	/* data being read or written */
	byte	n;	/* byte count */
	Rendez	r;
} i2c;

enum {
	/* i2c.state */
	Idle,
	Done,
	Failed,
	Start,
	Address,
/*	Subaddress, */	/* don't need them yet */
	Read,
	Write,
	/* Stop isn't a state */
};

void
i2cinit(void)
{
	byte r, s;

	// print("i2c\n");
	s = splhi();
	r = CLOCKFREQ/(8*Freq) - 2;
	if(r < 10)
		r = 10;
	iomem.ddrd &= ~3;	/* set pull-ups */
	iomem.portd |= 3;
	iomem.twar = 0;
	iomem.twbr = r;
	iomem.twsr = Twps1;
	i2c.state = Idle;
	splx(s);
}

void
i2cdisable(void)
{
	while(iomem.twcr & Twsto){
		/* wait */
	}
	iomem.twcr = 0;
	i2c.state = Idle;
}

static byte
i2cdone(void*)
{
	return i2c.state <= Failed;
}

static int
i2cio(int n)
{
	iomem.twcr = Twint | Twen | Twie | Twsta;
	sleep(&i2c.r, i2cdone, nil);
	if(i2c.state == Failed)
		n = -1;
	else
		n -= i2c.n;
	i2cdisable();
	return n;
}

int
i2csend(byte addr, byte *data, int n)
{
	if(n <= 0)
		return n;
	i2c.state = Start;
	i2c.slave = addr | Wbit;
	i2c.rw = Write;
	i2c.d = data;
	i2c.n = n;
	return i2cio(n);
}

int
i2crecv(byte addr, byte *data, int n)
{
	if(n <= 0)
		return n;
	i2c.state = Start;
	i2c.slave = addr | Rbit;
	i2c.rw = Read;
	i2c.d = data;
	i2c.n = n;
	return i2cio(n);
}

void
i2cintr(void)
{
	byte s, a;

	s = iomem.twsr & Tws;
	if(Trace)
		print("%d:%.2ux\n", i2c.state, s);
	switch(i2c.state){
	case Start:
		if(s != Tstart)
			goto Error;
		i2c.state = Address;
		iomem.twdr = i2c.slave;
		iomem.twcr = Twint | Twen | Twie;
		break;
	case Address:
		if(s != WslaAck && s != RslaAck)
			goto Error;
		i2c.state = i2c.rw;
		if(i2c.state == Write)
			goto W;
		goto R;
	case Read:
		if(s != RdataAck && s != RdataNak)
			goto Error;
		*i2c.d++ = iomem.twdr;
		i2c.n--;
	R:
		if(i2c.n == 0)
			goto Done;
		a = 0;
		if(i2c.n != 1)
			a = Twea;	/* don't ack last byte */
		iomem.twcr = Twint | Twen | Twie | a;
		break;
	case Write:
		if(s != WdataAck)
			goto Error;
	W:
		if(i2c.n == 0)
			goto Done;
		i2c.n--;
		iomem.twdr = *i2c.d++;
		iomem.twcr = Twint | Twen | Twie;
		break;
	default:
		goto Error;
	}
	return;

Done:
	i2c.state = Done;
	wakeup(&i2c.r);
	iomem.twcr = Twint | Twen | Twsto;
	return;

Error:
	if(Trace)
		print("i2c: bus error: state %d s=%.2ux\n", i2c.state, s);
	diag(Di2c);
	iomem.twcr = Twint | Twen;
	i2c.state = Failed;
	wakeup(&i2c.r);
}
