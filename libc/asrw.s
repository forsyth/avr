TEXT	_asrw(SB), $0
	MOVB	2(FP), R4
	CMPB	R4, R2
	BGE		asrw2
asrw1:
	ASRB	R21
	RORB	R20
	DEC		R4
	BNE		asrw1
asrw2:
	RET
