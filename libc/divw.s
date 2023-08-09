TEXT	_divww(SB), $0
	CMPW	R2, R18
	BNE		divww1
	CALL	zerodiv(SB)
	RET
divww1:
	CMPW	R20, R18
	BSL		divww2
	MOVW	R20, R18
	CLRW	R20
	RET
divww2:
	CLRW	R16
	MOVB	$16, R4
divww5:
	CMPB	R2, R21
	BNE		divww3
	MOVB	R20, R21
	CLRB	R20
	SUBB	$8, R4
	BNE		divww5
	RET
divww3:
	LSLW	R16
	CMPW	R2, R20
	BGE		divww6
	ORB		$1, R16
divww6:
	LSLW	R20
	CMPW	R16, R18
	BHI		divww4
	ORB		$1, R20
	SUBW	R18, R16
divww4:
	DEC		R4
	BNE		divww3
	MOVW	R16, R18
	RET

TEXT	_divw(SB), $0
	MOVW	2(FP), R18
	CMPW	R2, R20
	BGE		divwqpos
	NEGW	R20
	CMPW	R2, R18
	BGE		divwneg
	NEGW	R18
divwpos:
	CALL	_divww(SB)
	RET
divwqpos:
	CMPW	R2, R18
	BGE		divwpos
	NEGW	R18
divwneg:
	CALL	_divww(SB)
	NEGW	R20
	RET

TEXT	_divwu(SB), $0
	MOVW	2(FP), R18
	CALL	_divww(SB)
	RET
