TEXT	_mull(SB), $0
	MOVL	R4, R16
	MOVL	4(FP), R24
	MULBU	R16, R24
	MOVW	R0, R4
	MULBU	R17, R25
	MOVW	R0, R6
	MULBU	R17, R24
	ADDB	R0, R5
	ADDBC	R1, R6
	ADDBC	R2, R7
	MULBU	R16, R25
	ADDB	R0, R5
	ADDBC	R1, R6
	ADDBC	R2, R7
	MULBU	R18, R24
	ADDB	R0, R6
	ADDBC	R1, R7
	MULBU	R16, R26
	ADDB	R0, R6
	ADDBC	R1, R7
	MULBU	R19, R24
	ADDB	R0, R7
	MULBU	R18, R25
	ADDB	R0, R7
	MULBU	R17, R26
	ADDB	R0, R7
	MULBU	R16, R27
	ADDB	R0, R7
	RET

TEXT	_mullu(SB), $0
	MOVL	R4, R16
	MOVL	4(FP), R24
	MULBU	R16, R24
	MOVW	R0, R4
	MULBU	R17, R25
	MOVW	R0, R6
	MULBU	R17, R24
	ADDB	R0, R5
	ADDBC	R1, R6
	ADDBC	R2, R7
	MULBU	R16, R25
	ADDB	R0, R5
	ADDBC	R1, R6
	ADDBC	R2, R7
	MULBU	R18, R24
	ADDB	R0, R6
	ADDBC	R1, R7
	MULBU	R16, R26
	ADDB	R0, R6
	ADDBC	R1, R7
	MULBU	R19, R24
	ADDB	R0, R7
	MULBU	R18, R25
	ADDB	R0, R7
	MULBU	R17, R26
	ADDB	R0, R7
	MULBU	R16, R27
	ADDB	R0, R7
	RET
