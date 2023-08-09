#define	SPH	0x3e
#define	SPL	0x3d
#define	UDR	0x0c
#define	BREAK	WORD	$0x9598

TEXT	_startup(SB), $-1
	MOVB	R0, R1
	MOVW	R0, R2
	MOVW	$0x1024, R24
	OUT	R24, $SPL
	OUT	R25, $SPH
	MOVB	$'h', R2
	OUT	R2, $UDR
	MOVB	$'e', R2
	OUT	R2, $UDR
	MOVB	$'l', R2
	OUT	R2, $UDR
	OUT	R2, $UDR
	MOVB	$'o', R2
	OUT	R2, $UDR
	MOVB	$'\n', R2
	OUT	R2, $UDR
	MOVB	R8, 4(Y)
	ADDW	$0x0102, R16
	ADDW	$33, R24
	MOVW	R16, R20
	MOVW	R24, R24
	SUBW	$0x0102, R18
	SUBW	$34, R24
	MOVW	R24, R24
	INC	R24
	DEC	R24
	MOVW	R18, R20
	CALL	main(SB)
	BREAK
	BR	0(PC)	/* gives incorrect result */

TEXT	main(SB), $0
	MOVW	$_other(SB), Z	/* gives incorrect result */
//	CALL	(Z)
	RET

TEXT	_other(SB), $0
	MOVW	$25, R0
	RET

TEXT	putc(SB), $0
	OUT	R4, $UDR
	RET

