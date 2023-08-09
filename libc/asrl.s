TEXT	_asrl(SB), $0
	MOVB	4(FP), R8
	CMPB	R8, R2
	BGE		asrl2
asrl1:
	ASRB	R7
	RORB	R6
	RORB	R5
	RORB	R4
	DEC		R8
	BNE		asrl1
asrl2:
	RET
