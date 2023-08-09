	ADDB	/* ADD */
	ADDBC	/* ADC */
	ADDL
	ADDLC
	ADDW	/* ADIW macro add word */
	ADDWC
	ANDB	/* includes ANDI */
	ANDL
	ANDW
	ASRB
	ASRL
	ASRW
	BCLR
	BLD
	BSET
	BST
	CALL	/* includes RCALL ICALL EICALL and CALL */
	CBI
	CBRB
	CBRL
	CBRW
	CLRB
	CLRL
	CLRW
	CMP		/* includes CPI */
	CMPC
	COMB
	COML
	COMW
	CPSE
	DEC
	EORB
	EORL
	EORW
	FMULSB
	FMULSUB
	FMULUB
	IN
	INC
	LSLB
	LSLL
	LSLW
	LSRB
	LSRL
	LSRW
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
	NEGB
	NEGL
	NEGW
	ORB	/* includes ORI */
	ORL
	ORW
	OUT
	POPB
	POPL
	POPW
	PUSHB
	PUSHL
	PUSHW
	RET
	RETI
	ROLB
	ROLL
	ROLW
	RORB
	RORL
	RORW
	SBI
	SBIC
	SBIS
	SBRC
	SBRS
	SUBB	/* includes SUBI */
	SUBC	/* includes SBCI */
	SUBW	/* includes SBIW */
	SWAP
	TST

	/* BRBS BRBC: */
	/* C (carry) Z (zero) N (negative) V (overflow) S (N ^ V) H (half carry) T (transfer) I (interrupt) */
	BR
	BEQ
	BNE
	BCS
	BCC
	BSH	/* same or higher */
	BLO
	BMI
	BPL
	BGE
	BLT
	BHC
	BHS
	BTC
	BTS
	BVC
	BVS
	BIC
	BIS
	BGT
	BHI
	BLE
	BRA
	/* SEC etc are done using #define to BSET/BCLR */
