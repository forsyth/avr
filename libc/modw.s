TEXT	_modw(SB), $0
	MOVW	2(FP), R18
	CMPW	R2, R18
	BGE		modw1
	NEGW	R18
modw1:
	CMPW	R2, R20
	BLT		modwneg
	CALL	_divww(SB)
	MOVW	R18, R20
	RET
modwneg:
	NEGW	R20
	CALL	_divww(SB)
	NEGW	R18
	MOVW	R18, R20
	RET

TEXT	_modwu(SB), $0
	MOVW	2(FP), R18
	CALL	_divww(SB)
	MOVW	R18, R20
	RET

