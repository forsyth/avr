TEXT	_lslb(SB), $0
	MOVB	2(FP), R4
	CMPB	R4, R2
	BGE		lslb2
lslb1:
	LSLB		R20
	DEC		R4
	BNE		lslb1
lslb2:
	RET

