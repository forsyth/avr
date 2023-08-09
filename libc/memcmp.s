TEXT	memcmp(SB), $0
	MOVW	R20, Z
	MOVW	2(FP), X
	MOVW	4(FP), R16
	MOVW	R2, R20
	CMPW	R2, R16
	BLE		memcmpret
memcmp1:
	MOVB	(Z)+, R4
	MOVB	(X)+, R8
	CMPB	R8, R4
	BEQ		memcmp3
	BLT		memcmp2
	MOVB	$1, R20
	RET
memcmp2:
	MOVW	$-1, R20
	RET
memcmp3:
	SUBW	$1, R16
	BNE		memcmp1
memcmpret:
	RET
	
