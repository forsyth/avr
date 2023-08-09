#define	SPH	0x3e
#define	SPL	0x3d
#define	UDR	0x0c
#define	REGFP	28
#define	REGZERO	2
#define	X	R26
#define	Y	R28
#define	Z	R30

#define	BREAK	WORD	$0x9598

TEXT	_startup(SB), $-1
	EORB	R(REGZERO), R(REGZERO)
	MOVB	R(REGZERO), R((REGZERO+1))
	MOVW	$512, R(REGFP)
	MOVW	$1024, R24
	OUT	R25, $SPH
	OUT	R24, $SPL
	CALL	main(SB)
	BREAK
	BR	0(PC)	/* gives incorrect result */

TEXT	putc(SB), $0
	OUT	R4, $UDR
	RET

TEXT	setlabel(SB), $0
//	MOVW	R4, Z
	POPB	R17
	POPB	R16
	IN	$SPL, R24
	IN	$SPH, R25
	MOVW	R24, 0(R4)
	MOVW	R(REGFP), 2(R4)
	MOVW	R16, 4(R4)
	MOVW	$0, R4
//	JMP	(Z)
	PUSHB	R16
	PUSHB	R17
	RET

TEXT gotolabel(SB), $0
//	MOVW	R4, Z
	MOVW	0(R4), R24
	MOVW	2(R4), R(REGFP)
	MOVW	4(R4), R16
	OUT	R25, $SPH
	OUT	R24, $SPL
	MOVW	$1, R4
//	JMP	(Z)
	PUSHB	R16
	PUSHB	R17
	RET
