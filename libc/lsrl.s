TEXT	_lsrl(SB), $0
	MOVB	4(FP), R8
	CMPB	R8, R2
	BGE		lsrl2
lsrl1:
	LSRB		R7
	RORB	R6
	RORB	R5
	RORB	R4
	DEC		R8
	BNE		lsrl1
lsrl2:	
	RET

