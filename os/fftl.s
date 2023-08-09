/* int	_fftop(int, int);	ie (x*y+-512)/1024 */
TEXT	_fftop(SB), $0
	MOVW	2(FP), R22
	MULB	R21, R23
	MOVW	R0, R18
	MULBU	R20, R22
	MOVW	R0, R16
	MULBUS	R22, R21
	SUBBC	R2, R19
	ADDB	R0, R17
	ADDBC	R1, R18
	ADDBC	R2, R19
	MULBUS	R20, R23
	SUBBC	R2, R19
	ADDB	R0, R17
	ADDBC	R1, R18
	ADDBC	R2, R19
	CMPB	R2, R19
	BGE		fft1
	ADDL	$511, R16
	BR		fft2
fft1:
	SUBB	$254, R17
	SUBBC	$255, R18
	SUBBC	$255, R19
fft2:
	LSRB		R19
	RORB	R18
	RORB	R17
	LSRB		R19
	RORB	R18
	RORB	R17
	MOVB	R17, R20
	MOVB	R18, R21
	RET

/* int	_fftopq(int, int);	ie (x*y+-512)/1024 */
TEXT	_fftopq(SB), $0
	MULB	R21, R23
	MOVW	R0, R18
	MULBU	R20, R22
	MOVW	R0, R16
	MULBUS	R22, R21
	SUBBC	R2, R19
	ADDB	R0, R17
	ADDBC	R1, R18
	ADDBC	R2, R19
	MULBUS	R20, R23
	SUBBC	R2, R19
	ADDB	R0, R17
	ADDBC	R1, R18
	ADDBC	R2, R19
	CMPB	R2, R19
	BGE		fft1q
	ADDL	$511, R16
	BR		fft2q
fft1q:
	SUBB	$254, R17
	SUBBC	$255, R18
	SUBBC	$255, R19
fft2q:
	LSRB		R19
	RORB	R18
	RORB	R17
	LSRB		R19
	RORB	R18
	RORB	R17
	MOVB	R17, R20
	MOVB	R18, R21
	RET

/* long	_fftsq(int);	ie x*x */
TEXT	_fftsq(SB), $0
	MULB	R21, R21
	MOVW	R0, R6
	MULBU	R20, R20
	MOVW	R0, R4
	MULBUS	R20, R21
	SUBBC	R2, R7
	LSLW	R0
	ADDB	R0, R5
	ADDBC	R1, R6
	ADDBC	R2, R7
	RET

/* void _fft(int *f, int *sp) */
/*
	f	0(FP)
	sp	2(FP)
	n	128
	i	R24
	j	R25
	k	R26
	i2	R27
	a	-2(SP)
	b	-4(SP)
	c	R8-R9
	s	R10-R11
	t/u/tmp	R4-R5/R6-R7/R30-R31
	fp	R12-R13
	fq	R14-R15
	_fftopq	R16-R23
*/
TEXT	_fft(SB), $4
	MOVW	R20, 0(FP)		/* f */
	MOVB	$1, R24		/* i = 1 */
_ffti1:
	MOVB	R24, R27		/* i2 = i<<1 */
	LSLB		R27
	MOVW	2(FP), R30		/* sp */
	MOVW	(R30)+, R4	/* b = *sp++ */
	MOVW	R4, -4(SP)
	MOVW	R30, 2(FP)
	MOVW	(R30), R20	/* t = *sp */
	MOVW	R20, R22
	CALL	_fftopq(SB)	/* R(t, t) */
	LSLW	R20			/* 2*result */
	MOVW	R20, -2(SP)	/* a = 2*result */
	MOVW	$1024, R8		/* c = SCALE */
	CLRW	R10			/* s = 0 */
	CLRB	R25			/* j = 0 */
_fftj1:
	MOVW	0(FP), R12		/* fp = f+2*j ie f+4*j */
	MOVB	R25, R4
	CLRB	R5
	LSLW	R4
	LSLW	R4
	ADDW	R4, R12
	MOVW	R12, R14		/* fq = fp+i2 ie fp+2*i2 */
	MOVB	R27, R4
	CLRB	R5
	LSLW	R4
	ADDW	R4, R14	
	MOVB	R25, R26		/* k = j */
_fftk1:
	MOVW	(R14), R4		/* load *fq */
	MOVW	2(R14), R6	/* load fq[1] */
	MOVW	R4, R20
	MOVW	R8, R22
	CALL	_fftopq(SB)	/* R(c, *fq) */
	MOVW	R20, R30		/* t = result */
	MOVW	R6, R20
	MOVW	R10, R22
	CALL	_fftopq(SB)	/* R(s, fq[1]) */
	SUBW	R20, R30		/* t -= result */
	MOVW	R6, R20
	MOVW	R8, R22
	CALL	_fftopq(SB)	/* R(c, fq[1]) */
	MOVW	R20, R6		/* u = result */
	MOVW	R4, R20
	MOVW	R10, R22
	CALL	_fftopq(SB)	/* R(s, *fq) */
	ADDW	R20, R6		/* u += result */
	MOVW	R30, R4		/* save t */
	MOVW	(R12), R20	/* load *fp */
	MOVW	2(R12), R22	/* load fp[1] */
	MOVW	R20, R16		/* *fq = *fp-t */
	SUBW	R4, R16
	MOVW	R16, (R14)
	MOVW	R22, R16		/* fq[1] = fp[1]-u */
	SUBW	R6, R16
	MOVW	R16, 2(R14)
	ADDW	R4, R20		/* *fp += t */
	MOVW	R20, (R12)
	ADDW	R6, R22		/* fp[1] += u */
	MOVW	R22, 2(R12)
	MOVB	R27, R4		/* fp += 2*i2 ie 4*i2 */
	CLRB	R5
	LSLW	R4
	LSLW	R4
	ADDW	R4, R12
	ADDW	R4, R14		/* fq += 2*i2 ie 4*i2 */
	ADDB	R27, R26		/* k += i2 */
	CMPB	$128, R26		/* k < n */
	BLO		_fftk1
	MOVW	-2(SP), R4		/* load a */
	MOVW	-4(SP), R6		/* load b */
	MOVW	R4, R20
	MOVW	R8, R22
	CALL	_fftopq(SB)	/* R(a, c) */
	MOVW	R20, R30		/* t = result */
	MOVW	R6, R20
	MOVW	R10, R22
	CALL	_fftopq(SB)	/* R(b, s) */
	ADDW	R20, R30		/* t += result */
	MOVW	R4, R20
	MOVW	R10, R22
	CALL	_fftopq(SB)	/* R(a, s) */
	SUBW	R20, R10		/* s -= result */
	MOVW	R6, R20
	MOVW	R8, R22
	CALL	_fftopq(SB)	/* R(b, c) */
	ADDW	R20, R10		/* s += result */
	SUBW	R30, R8		/* c -= t */
	INC		R25			/* j++ */
	CMPB	R24, R25		/* j < i */
	BLO		_fftj1
	MOVB	R27, R24		/* i = i2 */
	CMPB	$128, R24		/* i < n */
	BLO		_ffti1
	RET
