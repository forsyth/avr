TEXT _mulbb(SB), $0
	CLRB	R20
	CMPB	R5, R4
	BGE		mulbb1
	EORB	R5, R4
	EORB	R4, R5
	EORB	R5, R4
mulbb1:
	SBRC	0, R5
	ADDB	R4, R20
	LSRB		R5
	BEQ		mulbb2
	LSLB		R4
	JMP		mulbb1
mulbb2:
	RET

TEXT	_mulb(SB), $0
	CALL	_mulbu(SB)
	RET

TEXT	_mulbu(SB), $0
	MOVB	R20, R4
	MOVB	2(FP), R5
	CALL	_mulbb(SB)
	RET
