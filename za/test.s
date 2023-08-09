TEXT	_startup(SB), $-1
	MOVB	R0, R1
	MOVW	R0, R2
	MOVB	R8, 4(Y)
	ADDW	$0x01020304, R16
	SUBW	$0x01020304, R18
	CALL	main(SB)
	BR	0(PC)

TEXT	main(SB), $0
	MOVW	$_other(SB), Z
	CALL	(Z)
	RET

TEXT	_other(SB), $0
	MOVW	$25, R0
	RET

