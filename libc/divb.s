TEXT _divbb(SB), $0
	CMPB	R2, R18
	BNE		divbb1
	CALL	zerodiv(SB)
	RET		
divbb1:
	CMPB	R20, R18
	BSL		divbb2
	MOVB	R20, R18
	CLRB	R20
	RET
divbb2:
	CLRB	R16
	MOVB	$8, R4
	CMPB	R2, R20
	BNE		divbb3
	RET
divbb3:
	LSLB		R16
	SBRC	7, R20
	ORB		$1, R16
	LSLB		R20
	CMPB	R16, R18
	BHI		divbb4
	ORB		$1, R20
	SUBB	R18, R16
divbb4:
	DEC		R4
	BNE		divbb3
	MOVB	R16, R18
	RET

TEXT	_divb(SB), $0
	MOVB	2(FP), R18
	CMPB	R2, R20
	BGE		divbqpos
	NEGB	R20
	CMPB	R2, R18
	BGE		divbneg
	NEGB	R18
divbpos:
	CALL	_divbb(SB)
	RET
divbqpos:
	CMPB	R2, R18
	BGE		divbpos
	NEGB	R18
divbneg:
	CALL	_divbb(SB)
	NEGB	R20
	RET

TEXT	_divbu(SB), $0
	MOVB	2(FP), R18
	CALL	_divbb(SB)
	RET
