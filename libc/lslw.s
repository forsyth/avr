TEXT	_lslw(SB), $0
	MOVB	2(FP), R4
	CMPB	R4, R2
	BGE		lslw2
lslw1:
	LSLB		R20
	ROLB	R21
	DEC		R4
	BNE		lslw1
lslw2:
	RET

