TEXT	_modl(SB), $0
	MOVL	4(FP), R8
	CMPL	$0, R8
	BGE		modl1
	NEGL	R8
modl1:
	CMPL	$0, R4
	BLT		modlneg
	CALL	_divll(SB)
	MOVL	R8, R4
	RET
modlneg:
	NEGL	R4
	CALL	_divll(SB)
	NEGL	R8
	MOVL	R8, R4
	RET

TEXT	_modlu(SB), $0
	MOVL	4(FP), R8
	CALL	_divll(SB)
	MOVL	R8, R4
	RET
