TEXT	memmove(SB), $0
TEXT	memcpy(SB), $0
	MOVW	R20, Z
	MOVW	2(FP), X
	MOVW	4(FP), R16
	CMPW	R2, R16
	BLE		memmoveret
	CMPW	X, Z
	BGT		memmoveback
memmove1:
	MOVB	(X)+, W
	MOVB	W, (Z)+
	SUBW	$1, R16
	BNE		memmove1
	RET
memmoveback:
	ADDW	R16, Z
	ADDW	R16, X
memmove2:
	MOVB	-(X), W
	MOVB	W, -(Z)
	SUBW	$1, R16
	BNE		memmove2
memmoveret:
	RET
