/*
 * IO memory
 */

typedef struct IOmem IOmem;

struct IOmem {
	byte	pinf;
	byte	pine;
	byte	ddre;
	byte	porte;
	byte	adcl;
	byte	adch;
	byte	adcsra;
	byte	admux;
	byte	acsr;
	byte	ubrr0l;
	byte	ucsr0b;
	byte	ucsr0a;
	byte	udr0;
	byte	spcr;
	byte	spsr;
	byte	spdr;
	byte	pind;
	byte	ddrd;
	byte	portd;
	byte	pinc;
	byte	ddrc;
	byte	portc;
	byte	pinb;
	byte	ddrb;
	byte	portb;
	byte	pina;
	byte	ddra;
	byte	porta;
	byte	eecr;
	byte	eedr;
	byte	eear[2];
	byte	sfior;
	byte	wdtcr;
	byte	ocdr;
	byte	ocr2;
	byte	tcnt2;
	byte	tccr2;
	byte	icr1l;
	byte	icr1h;
	byte	ocr1bl;
	byte	ocr1bh;
	byte	ocr1al;
	byte	ocr1ah;
	byte	tcnt1l;
	byte	tcnt1h;
	byte	tccr1b;
	byte	tccr1a;
	byte	assr;
	byte	ocr0;
	byte	tcnt0;
	byte	tccr0;
	byte	mcucsr;
	byte	mcucr;
	byte	tifr;
	byte	timsk;
	byte	eifr;
	byte	eimsk;
	byte	eicrb;
	byte	rampz;
	byte	xdiv;
	byte	spl;
	byte	sph;
	byte	sreg;
	byte	_pad0[0x40-0x3F];
	byte	ddrf;
	byte	portf;
	byte	ping;
	byte	ddrg;
	byte	portg;
	byte	_pad1[2];
	byte	spmcsr;
	byte	_pad2;
	byte	eicra;
	byte	_pad3;
	byte	xmcrb;
	byte	xmcra;
	byte	_pad4;
	byte	osccal;
	byte	twbr;
	byte	twsr;
	byte	twar;	
	byte	twdr;
	byte	twcr;
	byte	_pad5[3];
	byte	ocr1cl;
	byte	ocr1ch;
	byte	tccr1c;
	byte	_pad6;
	byte	etifr;
	byte	etimsk;
	byte	_pad7[2];
	byte	icr3l;
	byte	icr3h;
	byte	ocr3cl;
	byte	ocr3ch;
	byte	ocr3bl;
	byte	ocr3bh;
	byte	ocr3al;
	byte	ocr3ah;
	byte	tcnt3l;
	byte	tcnt3h;
	byte	tccr3b;
	byte	tccr3a;
	byte	tccr3c;
	byte	_pad8[3];
	byte	ubbrr0h;
	byte	_pad9[4];
	byte	ucsr0c;
	byte	_pad10[2];
	byte	ubbr1h;
	byte	ubbr1l;
	byte	ucsr1b;
	byte	ucsr1a;
	byte	udr1;
	byte	ucsr1c;
	byte	_pad11[0xDE-0x7D];
};


//#define	iomem	(*(IOmem*)0x20)

extern IOmem _iomem;
#define	iomem	_iomem

/* leds */
enum {
	Yellow = 1<<0,
	Green = 1<<1,
	Red = 1<<2
};

/* pseudo-DMA */
typedef struct Dma Dma;
struct Dma {
	byte*	src;
	byte*	dst;
	byte	inc;	/* increment */
	ushort	nbytes;
};
