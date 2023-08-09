TEXT	_asrb(SB), $0
	MOVB	2(FP), R4
	CMPB	R4, R2
	BGE		asrb2
asrb1:
	ASRB	R20
	DEC		R4
	BNE		asrb1
asrb2:
	RET

