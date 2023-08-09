TEXT	_modb(SB), $0
	MOVB	2(FP), R18
	SBRC	7, R18
	NEGB	R18
	CMPB	R2, R20
	BLT		modbneg
	CALL	_divbb(SB)
	MOVB	R18, R20
	RET
modbneg:
	NEGB	R20
	CALL	_divbb(SB)
	NEGB	R18
	MOVB	R18, R20
	RET

TEXT	_modbu(SB), $0
	MOVB	2(FP), R18
	CALL	_divbb(SB)
	MOVB	R18, R20
	RET
