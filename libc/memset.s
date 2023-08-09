TEXT	memset(SB), $0
	MOVW	R20, Z
	MOVW	2(FP), R4
	MOVW	4(FP), R16
	CMPW	R2, R16
	BLE		memsetret
memset1:
	MOVB	R4, (Z)+
	SUBW	$1, R16
	BNE		memset1
memsetret:
	RET
