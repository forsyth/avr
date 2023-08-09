TEXT	_divll(SB), $0
	CMPL	$0, R8
	BNE		divll1
	CALL	zerodiv(SB)
	RET
divll1:
	CMPL	R4, R8
	BSL		divll2
	MOVL	R4, R8
	CLRL		R4
	RET
divll2:
	CLRL		R16
	MOVB	$32, R20
divll5:
	CMPB	R2, R7
	BNE		divll3
	MOVB	R6, R7
	MOVB	R5, R6
	MOVB	R4, R5
	CLRB	R4
	SUBB	$8, R20
	BNE		divll5
	RET
divll3:
	LSLL		R16
	CMPL	$0, R4
	BGE		divll6
	ORB		$1, R16
divll6:
	LSLL		R4
	CMPL	R16, R8
	BHI		divll4
	ORB		$1, R4
	SUBL		R8, R16
divll4:
	DEC		R20
	BNE		divll3
	MOVL	R16, R8	
	RET

TEXT	_divl(SB), $0
	MOVL	4(FP), R8
	CMPL	$0, R4
	BGE		divlqpos
	NEGL	R4
	CMPL	$0, R8
	BGE		divlneg
	NEGL	R8
divlpos:
	CALL	_divll(SB)
	RET
divlqpos:
	CMPL	$0, R8
	BGE		divlpos
	NEGL	R8
divlneg:
	CALL	_divll(SB)
	NEGL	R4
	RET

TEXT	_divlu(SB), $0
	MOVL	4(FP), R8
	CALL	_divll(SB)
	RET

