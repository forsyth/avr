TEXT	_lsrb(SB), $0
	MOVB	2(FP), R4
	CMPB	R4, R2
	BGE		lsrb2
lsrb1:
	LSRB		R20
	DEC		R4
	BNE		lsrb1
lsrb2:
	RET

