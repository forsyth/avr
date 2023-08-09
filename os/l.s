#include "mem.h"

TEXT	_startup(SB), $-1
	EORB	R(REGZERO), R(REGZERO)
	MOVB	R(REGZERO), R((REGZERO+1))
/*
 * set initial stack: SP is hardware call stack; Y is first frame
 */
	MOVW	$(SRAMZERO+SRAMSIZE-1), Y
	OUT	YH, $SPH
	OUT	Y, $SPL
	SUBW	$HWSTACK, Y
/*
 * leds for debugging
 */
	MOVB	$0x7, R16
	OUT	R16, $DDRA
	OUT	R16, $PORTA
/*
 * copy initialised data
 */
	MOVW	$etext(SB), Z
	LSLW	Z	/* etext is in words */
	MOVW	$edata(SB), R16
	MOVW	$bdata(SB), X
copydata:
	CMPW	X, R16
	BEQ	donedata
	MOVPM	(Z)+, R18
	MOVB	R18, (X)+
	BR	copydata
donedata:
/*
 * clear bss
 */
	MOVW	$edata(SB), Z
	MOVW	$end(SB), X
clearbss:
	CMPW	X, Z
	BEQ		donebss
	MOVW	R2, (Z)+
	BR		clearbss
donebss:
	MOVB	$(1<<5|1<<2), R16	/* 1<<2 for ADC noise reduction mode */
	OUT	R16, $MCUCR	/* enable sleep mode */
	CALL	main(SB)
	BREAK

TEXT	idlehands(SB), $0
	BSET	7
	SLEEP
	WORD	$0
	WORD	$0
	BCLR 7
	RET

TEXT	splhi(SB), $0
	IN	$SREG, R(REGRET)
	ANDB	$(1<<7), R(REGRET)
	BCLR	7
	RET

TEXT	spllo(SB), $0
	IN	$SREG, R(REGRET)
	ANDB	$(1<<7), R(REGRET)
	BSET	7
	RET

TEXT	splx(SB), $0
	ORB	R(REGARG), R(REGARG)
	SBRC	7, R(REGARG); BSET 7
	SBRS		7, R(REGARG); BCLR 7
	RET

TEXT	setlabel(SB), $0
	POPB	R17
	POPB	R16
	IN	$SPL, R24
	IN	$SPH, R25
	MOVW	R(REGARG), Z
	MOVW	R24, 0(Z)
	MOVW	R(REGFP), 2(Z)
	MOVW	R16, 4(Z)
	MOVW	$0, R(REGRET)
	PUSHB	R16
	PUSHB	R17
	RET

TEXT gotolabel(SB), $0
	MOVW	R(REGARG), Z
	MOVW	0(Z), R24
	MOVW	2(Z), R(REGFP)
	MOVW	4(Z), R16
	OUT	R25, $SPH
	OUT	R24, $SPL
	MOVW	$1, R(REGRET)
	PUSHB	R16
	PUSHB	R17
	RET

TEXT	uart0init(SB), $0
#ifdef NONEED
	OUT	R(REGZERO), $UBRR0H
#endif
	MOVB	$15, R16	/* 57.6k */
	OUT	R16, $UBRR0L
	MOVB	$(1<<1), R16	/* double speed */
	OUT	R16, $UCSR0A
#ifdef NONEED
	MOVB	$(3<<1), R16	/* async, no parity, 1 stop, 8-bit */
	OUT	R16, $UCSR0C
#endif
	MOVB	$((1<<4)|(1<<3)), R16	/* enable RX, TX */
	OUT	R16, $UCSR0B
	RET

TEXT	uartputc(SB), $0
uartwait:
	SBIS	5, $UCSR0A; BR uartwait
	OUT	R(REGARG), $UDR0
	RET

TEXT	ledinit(SB), $0
	MOVB	$0x7, R16
	OUT	R16, $DDRA
	RET

TEXT	ledset(SB), $0
	IN	$PORTA, R16
	ANDB	$(0xFF&~7), R16
	ORB	R(REGARG), R16
	OUT	R16, $PORTA
	RET

TEXT	ledtoggle(SB), $0
	IN	$PORTA, R16
	EORB	R(REGARG), R16
	OUT	R16, $PORTA
	RET

TEXT	wait250ns(SB), $0
	WORD	$0
	WORD	$0
	RET

TEXT	waitusec(SB), $0
wait0:
	CMPW	$0, R(REGARG)
	BLE	wait1
	WORD	$0; WORD $0; WORD $0; WORD $0
	WORD	$0; WORD $0; WORD $0; WORD $0
	SUBW	$1, R(REGARG)
	BR	wait0
wait1:
	RET

/*
TEXT	_isave(SB), $-1
isave:
	// R31:R30 pushed by interrupt vector
	PUSHB	R29
	PUSHB	R28
	PUSHB	R27
	PUSHB	R26
	PUSHB	R25
	PUSHB	R24
	PUSHB	R23
	PUSHB	R22
	PUSHB	R21
	PUSHB	R20
	PUSHB	R19
	PUSHB	R18
	PUSHB	R17
	PUSHB	R16
	PUSHB	R15
	PUSHB	R14
	PUSHB	R13
	PUSHB	R12
	PUSHB	R11
	PUSHB	R10
	PUSHB	R9
	PUSHB	R8
	PUSHB	R7
	PUSHB	R6
	PUSHB	R5
	PUSHB	R4
	// skip R3:R2 (REGZERO)
	PUSHB	R1
	PUSHB	R0
	IN	$SREG, R(REGTMP)
	PUSHB	R(REGTMP)

	CALL	(Z)	// call the interrupt handler

	POPB	R(REGTMP)
	OUT	R(REGTMP), $SREG
	POPB	R0
	POPB	R1
	// skip R3:R2 (REGZERO)
	POPB	R4
	POPB	R5
	POPB	R6
	POPB	R7
	POPB	R8
	POPB	R9
	POPB	R10
	POPB	R11
	POPB	R12
	POPB	R13
	POPB	R14
	POPB	R15
	POPB	R16
	POPB	R17
	POPB	R18
	POPB	R19
	POPB	R20
	POPB	R21
	POPB	R22
	POPB	R23
	POPB	R24
	POPB	R25
	POPB	R26
	POPB	R27
	POPB	R28
	POPB	R29
	POPB	R30
	POPB	R31
	RETI
*/

TEXT	getcallerpc(SB), $-1
	IN	$SPH, ZH
	IN	$SPL, Z
	MOVW	2(Z), R(REGRET)
	RET

TEXT	getsp(SB), $-1
	IN	$SPH, R((REGRET+1))
	IN	$SPL, R(REGRET)
	RET

/*
 * must be in assembler to meet the 4-cycle deadline
 * until the compiler knows about SBI
 * (which means knowing about IO space)
 */
TEXT	eestartw(SB), $-1
	SBI	2, $0x1C	/* EEMWE */
	SBI	1, $0x1C	/* EEWE */
	RET

TEXT	adcgetw(SB), $-1
	IN	$4, R(REGRET)	/* ADCL */
	IN	$5, R((REGRET+1))	/* ADCH */
	RET

TEXT _intradcintr(SB), $-1
	PUSHL	R24
	PUSHW	R30
	IN		$SREG, R31
	PUSHB	R31
	MOVW	adc+6(SB), R30		/* a */
	IN		$4, R24			/* ADCL */
	IN		$5, R25			/* ADCH */
	MOVW	6(R30), R26
	CMPW	R2, R26
	BEQ		adcret
	MOVW	R24, (R26)+
	MOVW	R26, 6(R30)		/* *a->vp++ = v */
	MOVW	8(R30), R26
	CMPW	R2, R26
	BEQ		adcskip
	PUSHW	R0
	PUSHL	R4
	PUSHL	R8
	PUSHL	R12
	PUSHL	R16
	PUSHL	R20
	PUSHW	R28
	MOVW	R24, R20
	CALL	0(R26)			/* call a->f(v) if not nil */
	POPW	R28
	POPL		R20
	POPL		R16
	POPL		R12
	POPL		R8
	POPL		R4
	POPW	R0
adcskip:
	/* use B not W on nsamp ! */
	MOVB	4(R30), R24
	SUBB	$1, R24
	MOVB	R24, 4(R30)		/* --a->nsamp */
	CMPB	R2, R24
	BNE		adcret
	MOVW	R2, 6(R30)		/* a->vp = nil */
	CBI		5, $6				/* adcsra &= ~Freerun */
	PUSHW	R0
	PUSHL	R4
	PUSHL	R8
	PUSHL	R12
	PUSHL	R16
	PUSHL	R20
	PUSHW	R28
	MOVW	R30, R20
	CALL	wakeup+0(SB)
	POPW	R28
	POPL		R20
	POPL		R16
	POPL		R12
	POPL		R8
	POPL		R4
	POPW	R0
adcret:
	POPB	R31
	OUT		R31, $SREG
	POPW	R30
	POPL		R24
	RETI

/*
 * pseudo random number generator using 15-bit LFSR,
 * taps at bit 14 and bit 0
 * 25.10.2000 Tim Boescke - t.boescke(at)tuhh.de
 * The length of the pseudo random sequence is 32767.
 * A 16 Bit LFSR could be used to increase this to 65535
 */

	DATA	seed15<>+0(SB)/2, $0x00C2
	DATA	mask15<>+0(SB)/2, $0x2391
	GLOBL seed15<>(SB), $2
	GLOBL mask15<>(SB), $2

TEXT srand15(SB), $-1
	MOVW	R(REGARG), seed15<>(SB)
	RET

TEXT	rand15(SB), $-1
	MOVW	seed15<>(SB), R16
	MOVB	R16, R19
	SBRC	6, R17; COMB R19	/* bit 0 xor bit 14 */
	LSRB	R19
	ROLB	R16
	ROLB	R17
	MOVW	R16, seed15<>(SB)
	MOVW	R16, R(REGRET)
	MOVW	mask15<>(SB), R8
	EORW	R8, R(REGRET)
	RORW	R(REGRET)
	RORW	R(REGRET)
	RET

TEXT	watchdogpat(SB), $-1
	WDR
	RET

TEXT	stkpc(SB), $-1
	IN	$SPL, R20
	IN	$SPH, R21
	RET

TEXT stkfp(SB), $-1
	MOVW	R28, R20
	RET

TEXT _intrspiintr(SB), $-1
	PUSHW	R20
	IN	$SREG, R21
	PUSHB	R21
	IN	$SPDR, R20		/* spigetc */
	MOVB	spioutbyte+0(SB), R21
	OUT	R21, $SPDR		/* spiputc */
	PUSHW	R0
	PUSHL	R4
	PUSHL	R8
	PUSHL	R12
	PUSHL	R16
	PUSHW	R22
	PUSHL	R24
	PUSHL	R28
	CALL	radiointr(SB)
	POPL		R28
	POPL		R24
	POPW	R22
	POPL		R16
	POPL		R12
	POPL		R8
	POPL		R4
	POPW	R0
	POPB	R21
	OUT	R21, $SREG
	POPW	R20
	RETI
