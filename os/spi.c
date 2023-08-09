#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

enum {
	/* spcr */
	Ienable=	1<<7,
	Enable=	1<<6,
	OrdLSB=	1<<5,	/* otherwise MSB */
	Master=	1<<4,	/* otherwise slave */
	Cpolhi=	1<<3,	/* otherwise low */
	CphaT=	1<<2,	/* sample trailing edge; otherwise sample leading edge */

	/* clocks; ignored in slave mode */
	Clk4=	0,
	Clk16,
	Clk64,
	Clk128,
	Clk2,
	Clk8,
	Clk32,
	/*Clk64, */

	/* spsr */
	Spif=	1<<7,
	Wcol=	1<<6,	/* write collision */
	Spi2x=	1<<0,	/* double clock in master mode */

	/* ports */
	B_ss_n_i=	1<<0,	/* slave select: SS# */
	B_sck_i=	1<<1,
	B_mosi_io=	1<<2,
	B_miso_io=	1<<3,
	B_oc1c_i=	1<<7,
};

void
spislave(void)
{
	byte s, b;

	s = splhi();
	iomem.ddrb &= ~(B_sck_i | B_oc1c_i | B_mosi_io | B_miso_io | B_ss_n_i);
	iomem.spcr = 0;	/* low polarity, sample leading edge, MSB */
	b = iomem.spsr;
	USED(b);
	b = iomem.spdr;
	USED(b);
	splx(s);
}

void
spioff(void)
{
	byte s;

	s = splhi();
	iomem.ddrb &= ~B_ss_n_i;
	iomem.portb &= ~B_ss_n_i;
	iomem.spcr = 0;
	splx(s);
}

void
spipause(void)
{
	iomem.spcr = 0;
}

void
spitx(void)
{
	byte s;

	s = splhi();
	iomem.spcr = Ienable | Enable;	/* low polarity, sample leading edge, MSB, Clk4 */
	iomem.ddrb |= B_miso_io;
	splx(s);
}

void
spirx(void)
{
	byte s;

	s = splhi();
	iomem.spcr = Ienable | Enable;	/* low polarity, sample leading edge, MSB, Clk4 */
	iomem.ddrb &= ~B_miso_io;
	splx(s);
}

int
spiready(void*)
{
	return iomem.spsr & Spif;
}
