TEXT	_lsll(SB), $0
	MOVB	4(FP), R8
	CMPB	R8, R2
	BGE		lsll2
lsll1:
	LSLB		R4
	ROLB	R5
	ROLB	R6
	ROLB	R7
	DEC		R8
	BNE		lsll1
lsll2:
	RET

