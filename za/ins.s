	TEXT	various(SB), $-1
	ADDB	$5, R0
	ADDB	R1, R2	/* ADD */
	ADDBC	R3, R4	/* ADC */
	ADDL	$5, R4
	ADDL	R6, R8
	ADDLC	R6, R8
	ADDW	$0x0102,	R16	/* ADIW macro add word */
	ADDWC	R16, R18
	ANDB	$0xFE, R3	/* includes ANDI */
	ANDB	R3, R5
	ANDL	$0x01020304, R16
	ANDW	$0x0102, R16
	ASRB	R0
	ASRL		R16
	ASRW	R16
	BCLR	0
	BLD		7, R1
	BSET		6
	BST		2, R1
	CALL	fred(SB)	/* includes RCALL ICALL EICALL and CALL */
	CALL	(Z)
	CBI		2, R7
	CBRB	$7, R4
	CBRL	$0xF0F1, R4
	CBRW	$0xF0F1, R4
	CLRB	R3
	CLRL		R4
	CLRW	R4
	CMPB		R4, R7	/* includes CPI */
	CMPB	$7, R7
	CMPBC	R4, R7
	CMPW	R0, R4
	CMPW	$12345, R4
	COMB	R3
	COML	R4
	COMW	R4
	CPSE		R4, R6
	DEC	R3
	EORB	R4, R6
	EORL	R4, R8
	EORW	R4, R6
#ifdef YYY
	FMULSB
	FMULSUB
	FMULUB
	IN
#endif
	INC	R3
	LSLB	R4
	LSLL	R4
	LSLW	R4
	LSRB	R4
	LSRL	R4
	LSRW	R4
#ifdef YYY
	MOVB	/* MOV LD LDD LDI LDS ST STD STS */
	MOVBL
	MOVBW
	MOVBZL
	MOVBZW
	MOVL
	MOVW
	MOVWL
	MOVWZL
	MULSB
	MULUB
#endif
	NEGB	R3
	NEGL	R4
	NEGW	R6
	ORB	$0xFF, R1	/* includes ORI */
	ORB	R3, R4
	ORL	$0xF0F0F1F2, R4
	ORW	$0xF0F1, R6
	OUT	R7, $0x35
	POPB	R3
	POPL	R4
	POPW	R6
	PUSHB	R3
	PUSHL	R4
	PUSHW	R6
	RET
	RETI
	ROLB	R3
	ROLL	R4
	ROLW	R4
	RORB	R3
	RORL	R4
	RORW	R4
	SBI	3, $0x15
	SBIC	3, $0x16
	SBIS	4, $0x17
	SBRC	7, R9
	SBRS	1, R23
	SUBB	R7, R6	/* includes SUBI */
	SUBB	$7, R6
	SUBBC	R7, R6	/* includes SBCI */
	SUBBC	$7, R6
	SUBW	$20000, R6	/* includes SBIW */
	SUBW	R4, R6
	SWAP	R3
	TST	R7
label:

	/* BRBS BRBC: */
	/* C (carry) Z (zero) N (negative) V (overflow) S (N ^ V) H (half carry) T (transfer) I (interrupt) */
	BR	label
	BEQ	label
	BNE	label
	BCS	label
	BCC	label
	BSH	label	/* same or higher */
	BLO	label
	BMI	label
	BPL	label
	BGE	label
	BLT	label
	BHC	label
	BHS	label
	BTC	label
	BTS	label
	BVC	label
	BVS	label
	BIC	label
	BIS	label
	BGT	label
	BHI	label
	BLE	label
	/* SEC etc are done using #define to BSET/BCLR */
	RET
