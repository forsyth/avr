#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

#define DPRINT	if(0)print

/*
 * mica2 pin assignments:
 *	CHP_OUT	A6	(input)
 *	PDATA	D7	(output)
 *	PCLK	D6	(output)
 *	PALE	D4	(output)
 */

enum {
	A_chp_out_i=	1<<6,
	D_pdata_o=	1<<7,
	D_pclk_o=	1<<6,
	D_pale_o=	1<<4,
};

/*
 * see the CC1000 SmartRF data sheet
 */
enum {
	/* main */
	Rmain=	0,
	  Rx=	0<<7,	/* Rx/Tx switch */
	  Tx=	1<<7,
	  F_regA=	0<<6,	/* select frequency register A/B */
	  F_regB=	1<<6,
	  Rx_PD=	1<<5,	/* power down LNA, mixer, IF, ... Rx part */
	  Tx_PD=	1<<4,	/* power down Tx part */
	  FS_PD=	1<<3,	/* power down frequency synthesiser */
	  Core_PD=	1<<2,	/* power down crystal osc. core */
	  Bias_PD=	1<<1,	/* power down bias */
	  Reset_N=	1<<0,	/* reset, active low */

	Rfreq2a=	1,	/* freq control A: bits 16-24 */
	Rfreq1a=	2,	/* bits 8-15 */
	Rfreq0a=	3,	/* bits 0-7 */
	Rfreq2b=	4,	/* freq control B */
	Rfreq1b=	5,
	Rfreq0b=	6,
	Rfsep1=	7,	/* 8-10 of frequency separation */
	Rfsep0=	8,	/* 0-7 of freq sep */

	Rcurrent=	9,
	  Vco150μA=	0<<4,
	  Vco250μA=	1<<4,
	  Vco350μA=	2<<4,
	  Vco450μA=	3<<4,
	  Vco950μA=	4<<4,	/* use for Rx, f=400-500 MHz */
	  Vco1050μA=	5<<4,
	  Vco1150μA=	6<<4,
	  Vco1250μA=	7<<4,
	  Vco1450μA=	8<<4,	/* use for Rx, f<400 && f>500, and Tx f=400-500 */
	  Vco1550μA=	9<<4,	/* use for Tx f<400 */
	  Vco1650μA=	10<<4,
	  Vco1750μA=	11<<4,
	  Vco2250μA=	12<<4,
	  Vco2350μA=	13<<4,
	  Vco2450μA=	14<<4,
	  Vco2550μA=	15<<4,	/* use for Tx f>500 */
	  Lo0_5mA=	0<<2,	/* control VCO buffer current, LO drive (use 0.5mA for Tx) */
	  Lo1_0mA=	1<<2,	/* use for Rx, f<500 */
	  Lo1_5mA=	2<<2,
	  Lo2_0mA=	3<<2,	/* use for Rx f>500 */
	  Pa1mA=		0<<0,	/* control of VCO buffer current, for PA (use 1mA for Rx) */
	  Pa2mA=		1<<0,	/* use for Tx, f<500 */
	  Pa3mA=		2<<0,
	  Pa4mA=		3<<0,	/* use for Tx, f>500 */

	Rfrontend=	10,
	  Buf520uA=	0<<5,	/* LNA_FOLLOWER current, use 520 for f<500 */
	  Buf690uA=	1<<5,	/* use for f>500 */
	  Lna0_8mA=	0<<3,	/* LNA current; use 0.8mA for f<500 */
	  Lna1_4mA=	1<<3,
	  Lna1_8mA=	2<<3,	/* for f>500 */
	  Lna2_2mA=	3<<3,
	  Rssi0=		0<<1,	/* internal IF and demod, RSSI inactive */
	  Rssi1=		1<<1,	/* RSSI active, RSSI/IF is analog RSSI output */
	  Rssi2=		2<<1,	/* external IF and dmod, RSSI/IF is mixer */
	  Xoscenable=	0<<0,	/* internal XOSC enabled */
	  Xoscext=	1<<0,	/* oscillator is external */

	Rpapow=		11,		/* control of power in high (bits 4-7) and low (bits 0-3) power arrays */
	Rpll=			12,
	  Extfilter=		1<<7,	/* external loop filter; =0, internal one */
#define	Refdiv(n)	((n)<<3)	/* reference divisor, 2 to 15 */
	  Alarmdisable= 1<<2,	/* alarm function disabled */
	  AlarmH=		1<<1,	/* voltage too close to VDD */
	  AlarmL=		1<<0,	/* voltage too close to GND */

	Rlock=		13,		/* select signals to CHP_OUT (LOCK) pin */
	  Lchp_out=	0<<4,	/* normal, pin can be used as CHP_OUT */
	  Lbadman=	9<<4,	/* manchester violation, active high */
	  Lock127=	0<<3,	/* lock threshold=127, reset lock=111 (worst accuracy 0.7%) */
	  Lock31=		1<<3,	/* lock threshold=31, reset lock=15 (worst accuracy 2.8%) */
	  Lockinstant=	1<<1,	/* status bit from lock detector */
	  Lockcontin=	1<<0,	/* status bit from lock detector */

	Rcal=		14,
	  Calstart=		1<<7,	/* calibration started */
	  Caldual=		1<<6,	/* store calibration in A and B (otherwise, use F_Reg[AB] in Rmain) */
	  Calwait=		1<<5,	/* normal calibration wait time (otherwise half time) */
	  Calcurx2=	1<<4,	/* calibration current doubled */
	  Caldone=	1<<3,	/* calibration complete */
	  Caliterate=	6<<0,	/* interation start value; this one is `normal' */

	Rmodem2=	15,
	  Peakdetect=	1<<7,	/* enable peak detector/remover */
						/* bits 0-6 are peak level offset (correlated to freq. deviation) */
	Rmodem1=	16,
#define	Mlimit(n)	((n)<<5)	/* manchester violation limit */
	  Lockavgin=	1<<4,	/* average filter is locked */
	  Lockauto=	0<<3,
	  Lockavg=	1<<3,	/* use Lockavgin */
	  					/* bits 1-2 are settling time of average filter */
	  Modemreset_n=	1<<0,	/* modem reset, active low */
	Rmodem0=	17,
	  Baud600=	0<<4,	/* 0.6kbaud */
	  Baud1200=	1<<4,
	  Baud2400=	2<<4,
	  Baud4800=	3<<4,
	  Baud9600=	4<<4,
	  Baud19200=	5<<4,
	  Baud38400=	5<<4,
	  DataNRZ=	0<<2,
	  DataMan=	1<<2,	/* manchester operation */
	  DataUART=	2<<2,	/* transparent asynchronous UART */
	  Xosc3_4MHz=	0<<0,
	  Xosc6_8MHz=	1<<0,	/* also used for 38.4k 14.7456MHz */
	  Xosc9_12MHz=	2<<0,
	  Xosc12_16MHz=	3<<0,
	Rmatch=		18,		/* sets matching capacitor array for Rx(4-7) and Tx(0-3), 0.4pF steps */
	Rfsctrl=		19,
	  Dither1=		1<<3,	/* enable dithering when transmitting 1 */
	  Dither0=		1<<2,	/* enable dithering when transmitting 0, and during Rx */
	  Shapen=		1<<1,	/* enable data shaping */
	  Shapereset_n= 1<<0,	/* shape sequencer reset, active low */

	Rfshape7=	20,
	Rfshape6,
	Rfshape5,
	Rfshape4,
	Rfshape3,
	Rfshape2,
	Rfshape1,

	Rfsdelay=		0x1B,	/* clock cycles delay between use of fshape registers during shaping */
	Rprescaler=	0x1C,	/* prescaler swing */

	Rtest6=		0x40,
	Rtest5,
	Rtest4,			/* L2KIO; charge pump current; use 0x3F for 38.4/76.8 kBaud */
	Rtest3,
	Rtest2,
	Rtest1,
	Rtest0
};

typedef struct CC1kconf CC1kconf;

struct CC1kconf {
	byte	regs[Rprescaler+1];
	byte	txpow;	/* Tx current */
	byte	islo;	/* high side LO? */
	byte	l2kio;	/* charge pump current */
};

/* 
  * tx frequencies 914.045 and 914.109 MHz (64 KHz apart)
  * rx frequency 914.077 MHz (middle of tx range)
  */
#ifdef OLD
static CC1kconf  conf914_077= {
	{
	[Rmain]	Tx_PD|FS_PD|Reset_N,
	[Rfreq2a]	0x5c,0xe0,0,					
	[Rfreq2b]	0x5c,0xdb,0x42,					
	[Rfsep1]	0x01,0xAA,
	[Rcurrent]	Vco1450μA | Lo2_0mA,	/* Rx current */
	[Rfrontend]	Buf690uA | Lna1_8mA | Rssi1,
	[Rpapow]	(8<<4) | (0<<0),
	[Rpll]	Refdiv(6),		
	[Rlock]	Lbadman,
	[Rcal]	Calwait | Caliterate,	
	[Rmodem2]	0,
	[Rmodem1]	Mlimit(3) | Lockavg | (3<<1) | Modemreset_n, 
	[Rmodem0]	Baud38400 | DataMan | Xosc6_8MHz,
	[Rmatch]	(1<<4) | (0<<0),	/* 2<<4 in prelim data sheet */
	[Rfsctrl]	Shapereset_n,			
	[Rfshape7]	0,0,0,0,0,0,0,	
	[Rfsdelay]	0,	
	[Rprescaler]	0,
	},
	Vco2550μA | Pa4mA,	/* Tx current */
	1,	/* high side LO */
	0x3F,	/* l2kio */
};
#endif

/* 
  * rx frequency 868.918800 MHz (middle of tx range)
  * in fact
  *	tx0 = 868.887
  *	tx1 = 868.951
  *	(t0+t1)/2 = 868.919
  *	rx = 869.069 
  */
static CC1kconf  conf868_919= {
	{
	[Rmain]	Tx_PD|FS_PD|Reset_N,
	[Rfreq2a]	0x75,0xc0,0x00,
	[Rfreq2b]	0x75, 0xb9, 0xae,
	[Rfsep1]	0x2, 0x38,
	[Rcurrent]	Vco1450μA | Lo2_0mA,	/* Rx current */
	[Rfrontend]	Buf690uA | Lna1_8mA | Rssi1,
	[Rpapow]	(8<<4) | (0<<0),	// 255
	[Rpll]	Refdiv(8),		
	[Rlock]	Lbadman,
	[Rcal]	Calwait | Caliterate,	
	[Rmodem2]	0,			// (1<<7) | 33,
	[Rmodem1]	Mlimit(3) | Lockavg | (3<<1) | Modemreset_n, 
	[Rmodem0]	Baud38400 | DataMan | Xosc6_8MHz,
	[Rmatch]	(1<<4) | (0<<0),	/* 2<<4 in prelim data sheet */
	[Rfsctrl]	Shapereset_n,			
	[Rfshape7]	0,0,0,0,0,0,0,	
	[Rfsdelay]	0,	
	[Rprescaler]	0,
	},
	Vco2550μA | Pa4mA,	/* Tx current */
	1,	/* high side LO */
	0x3F,	/* l2kio */
};

#define	conf_freq	conf868_919

void
cc1kreset(void)
{
	byte i;

	for(i = 0; i < 100; i++)
		waitusec(10000);

	iomem.ddra &= ~A_chp_out_i;
	iomem.ddrd |= D_pdata_o | D_pclk_o | D_pale_o;
	iomem.portd |= D_pdata_o | D_pclk_o | D_pale_o;
	cc1kputreg(Rmain, F_regA|Rx_PD|Tx_PD|FS_PD|Bias_PD);	/* initial values with RESET low, then high */
	cc1kputreg(Rmain, F_regA|Rx_PD|Tx_PD|FS_PD|Bias_PD|Reset_N);
	waitusec(2500);	 /* wait 2ms */
	/* program all regs but main: F_regA for Rx, F_regB for Tx */
	cc1kload();
	cc1kdump();
		/* set other defaults */
		/* set Rpapow, Rlock, Rmodem[210] */
		/* set F/A, F/B, Rfsep, Rcurrent, Rfrontend, Rpower, Rpll, Rmatch */
		/* set TEST4 */

	/* calibrate VCO and PLL */
	cc1kcal();

	/* set main for power down */
	cc1ksleep();
}

void
cc1kputreg(byte addr, byte data)
{
	byte i, s;
	IOmem *io;

	/* clock out the address */
	io = &iomem;
	s = splhi();
	io->portd &= ~D_pale_o;
	for(i = 0; i < 7; i++){
		addr <<= 1;
		if(addr & 0x80)
			io->portd |= D_pdata_o;
		else
			io->portd &= ~D_pdata_o;
		io->portd &= ~D_pclk_o;
		io->portd |= D_pclk_o;
	}
	io->portd |= D_pdata_o;	/* high, for write */
	io->portd &= ~D_pclk_o;
	io->portd |= D_pclk_o;
	io->portd |= D_pale_o;

	/* clock out the data */
	for(i = 0; i < 8; i++){
		if(data & 0x80)
			io->portd |= D_pdata_o;
		else
			io->portd &= ~D_pdata_o;
		io->portd &= ~D_pclk_o;
		io->portd |= D_pclk_o;
		data <<= 1;
	}
	io->portd |= D_pdata_o;
	splx(s);
}

byte
cc1kgetreg(byte addr)
{
	byte i, s, data;
	IOmem *io;

	/* clock out the address */
	io = &iomem;
	s = splhi();
	io->portd &= ~D_pale_o;
	for(i = 0; i < 7; i++){
		addr <<= 1;
		if(addr & 0x80)
			io->portd |= D_pdata_o;
		else
			io->portd &= ~D_pdata_o;
		io->portd &= ~D_pclk_o;
		io->portd |= D_pclk_o;
	}
	io->portd &= ~D_pdata_o;	/* low, for read */
	io->portd &= ~D_pclk_o;
	io->portd |= D_pclk_o;
	io->portd |= D_pale_o;

	/* clock in the data */
	io->ddrd &= ~D_pdata_o;	/* now it's input */
	data = 0;
	for(i = 0; i < 8; i++){
		io->portd &= ~D_pclk_o;
		data <<= 1;
		if(io->pind & D_pdata_o)
			data |= 1;
		io->portd |= D_pclk_o;
	}
	io->portd |= D_pdata_o;
	io->ddrd |= D_pdata_o;
	splx(s);
	return data;
}

static void
cc1kcal1(char *v)
{
	byte n;

	DPRINT("calibrate\n");
	cc1kputreg(Rcal, Calwait | Caliterate | Calstart);
	for(n=0; (cc1kgetreg(Rcal) & Caldone) == 0; n++){
		/* wait */
		waitusec(250);
	}
	cc1kputreg(Rcal, 0);
	DPRINT("%s: n=%d %.2ux %.2ux\n", v, n, cc1kgetreg(Rtest2), cc1kgetreg(Rtest0));
}

void
cc1kcal(void)
{
	byte pow;

	/* page 23 */
	pow = cc1kgetreg(Rpapow);
	cc1kputreg(Rcal, 0);

	/* Rx frequency register A is calibrated first */
	cc1kputreg(Rmain, Rx | F_regA | Tx_PD | Reset_N);
	cc1kputreg(Rcurrent, conf_freq.regs[Rcurrent]);
	cc1kcal1("Rx");

	/* Tx frequency register B is calibrated second */
	cc1kputreg(Rmain, Tx | F_regB | Rx_PD | Reset_N);
	cc1kputreg(Rcurrent, conf_freq.txpow);
	cc1kputreg(Rpapow, 0);	/* turn off rf amp */
	cc1kcal1("Tx");

	cc1kputreg(Rpapow, pow);
}

/*
 * see the flow chart on page 27 of the datasheet
 */

void
cc1kwake(void)
{
	cc1kputreg(Rmain, Rx_PD | Tx_PD | FS_PD | Bias_PD | Reset_N);
	waitusec(2000);
//	cc1kbias(1);
	// cc1kputreg(Rfrontend, conf_freq.regs[Rfrontend]);
}

void
cc1kbias(byte on)
{
	if(on){
		cc1kputreg(Rmain, Rx_PD | Tx_PD | FS_PD | Reset_N);
		waitusec(200);
	}else
		cc1kputreg(Rmain, Rx_PD | Tx_PD | FS_PD | Bias_PD | Reset_N);
}

void
cc1ktx(void)
{
	cc1kputreg(Rmain, Tx | F_regB | Rx_PD | Reset_N);
	cc1kputreg(Rcurrent, conf_freq.txpow);
	waitusec(250);
	cc1kputreg(Rpapow, conf_freq.regs[Rpapow]);
	waitusec(20);
}

void
cc1krx(void)
{
	cc1kputreg(Rmain, Rx | F_regA | Tx_PD | Reset_N);
	cc1kputreg(Rcurrent, conf_freq.regs[Rcurrent]);
	cc1kputreg(Rpapow, 0);
	waitusec(250);
}

void
cc1ksleep(void)
{
	cc1kputreg(Rpapow, 0);
	cc1kputreg(Rmain, Rx_PD | Tx_PD | FS_PD | Core_PD | Bias_PD | Reset_N);
	// cc1kputreg(Rfrontend, Buf690uA | Lna1_8mA | Rssi0);
}

byte
cc1kgetlock(void)
{
	return iomem.pina & A_chp_out_i;
}

void
cc1ksetlock(byte v)
{
	USED(v);	/* TO DO? */
}

void
cc1kload(void)
{
	byte i, c;

	for(i=Rmain+1; i<nelem(conf_freq.regs); i++){
		c = conf_freq.regs[i];
		cc1kputreg(i, c);
	}
	c = conf_freq.l2kio;
	if(c)
		cc1kputreg(Rtest4, c);
}

void
cc1kdump(void)
{
	byte i;

	for(i=0; i<0x44; i++)
		DPRINT("%.2ux: %.2ux\n", i, cc1kgetreg(i));
}
