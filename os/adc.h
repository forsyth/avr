typedef struct Adc Adc;

enum {
	/* adcsra */
	Scale2=	1,
	Scale4=	2,
	Scale8=	3,
	Scale16=	4,
	Scale32=	5,
	Scale64=	6,
	Scale128=	7,

	/* admux */
	RefAREF=	0<<6,
	RefAVCC=	1<<6,
	Ref2_56v= 3<<6,
	Leftjust=	1<<5,
	/* low order 5 bits: 00 xxx selects single-ended source xxx */
	/* then various differential combinations */
	Muxbg=	0x1E,	/* bandgap */
	Mux0V=	0x1F,
	ADC0=	0,
	ADC1,
	ADC2,
	ADC3,
	ADC4,
	ADC5,
	ADC6,
	ADC7,
	Nadc,

	AdcFirstTime=	25,	/* ADC clock cycles to first conversion */
	AdcNormTime=	13,	/* ADC clock cycles to second and subsequent conversions (single-ended) */
	/* can be 14 cycles for differential */
};

/* _intradcintr in l.s knows about this struct */
struct Adc {
	Rendez	r;
	byte	dev;
	byte	scale;
	ushort	nsamp;	/* samples to take */
	ushort	*vp;		/* sample buffer */
	void	(*f)(ushort);
};

extern	byte	adcacquire(Adc*, byte);
extern	Adc*	adcalloc(byte, byte);
extern	void	adcdisable(void);
extern	void	adcenable(Adc*);
extern	ushort	adcgetw(void);
extern	void	adcinit(void);
extern	byte	adcisbusy(void);
extern	ushort	adcread(Adc*);
extern	ushort	adcread1(Adc*);
extern	void	adcrelease(Adc*);
extern	void	adcsample(Adc*, ushort, ushort*);
extern	ushort	adcscale(ushort, byte*);
extern	byte	adcstart(Adc*, byte);
extern	void	adcstop(Adc*);
extern	void	adctrigger(Adc*, void (*f)(ushort));
