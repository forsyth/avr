TEXT	_lsrw(SB), $0
	MOVB	2(FP), R4
	CMPB	R4, R2
	BGE		lsrw2
lsrw1:
	LSRB		R21
	RORB	R20
	DEC		R4
	BNE		lsrw1
lsrw2:
	RET

