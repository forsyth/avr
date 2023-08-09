/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */
#define	BI2BY	8	/* bits per byte */
#define	BI2WD	16	/* bits per word */
#define	BY2WD	2	/* bytes per word */
#define	ROUND(s, sz) (((s)+((sz)-1))&~((sz)-1))
#define	BLOCKALIGN	2

#define	EEPROMSIZE	4096
#define	SRAMZERO	0x100	/* physical address of SRAM, beyond registers and IO */
#define	SRAMSIZE	4096
#define	UREGSIZE	(30+1+2)	/* 30 regs (not REGZERO), SREG, SP */
#define	MAXCALLS	25	/* limit to call depth */
#define	HWSTACK	(MAXCALLS*BY2WD+UREGSIZE)	/* size of hardware stacks, including Ureg save area */
#define	SWSTACK	(MAXCALLS*8)
#define	STACKSIZE	(HWSTACK+SWSTACK)
#define	MAINSTACK	(HWSTACK+45*BY2WD)	/* smaller stack reserved for main (calls sched) */

#define	REGFP	28	/* Y */
#define	REGZERO	2
#define	REGRET	20
#define	REGTMP	22
#define	REGARG	REGRET

#define	SPH	0x3e
#define	SPL	0x3d
#define	SREG	0x3f

#define	UBRR0L	0x09
#define	UCSR0A	0x0B
#define	UCSR0B	0x0A
#define	UDR0	0x0c
#define	PORTA	0x1B
#define	DDRA	0x1A
#define	PINA	0x19
#define	MCUCR	0x35
#define	SPDR	0x0f

#define	CLOCKFREQ	7372800L
