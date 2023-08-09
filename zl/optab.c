#include	"l.h"

Optab	optab[] =
{
	/* as		a1	a2 		type  size	version */
	{ ATEXT,	C_LEXT,	C_LCON,	 	 0, 0 },
	{ ANOOP,		C_REG,	C_NONE,		0, 0 },
	{ ANOOP,		C_NONE,	C_REG,		0, 0 },
	{ ANOOP,		C_REG,	C_REG,		0, 0 },
	{ ANOOP,		C_NONE,	C_NONE,	 	 0, 0 },

	/* reg, reg byte */
	{ AMOVB,	C_REG,	C_REG,		 1, 2 },
	{ AADDB,	C_REG,	C_REG,		 1, 2 },
	{ ASUBB,	C_REG,	C_REG,		 1, 2 },
	{ AANDB,	C_REG,	C_REG,		 1, 2 },
	{ AEORB,	C_REG,	C_REG,		 1, 2 },
	{ AMULUB,C_REG,	C_REG,		1, 2 },
	{ AMULB,	C_HREG,	C_HREG,		1, 2 },
	{ ACMPB,	C_REG,	C_REG,		1, 2 },
	{ ACMPSE,	C_REG,	C_REG,		1, 2 },

	/* imm, reg byte */
	{ ASUBB,	C_KCON,	C_HREG,		2, 2 },
	{ AANDB,	C_KCON,	C_HREG,		 2, 2 },
	{ AMOVB,	C_KCON, C_HREG,		 2, 2 },
	{ ACBRB,	C_KCON,	C_HREG,		2, 2 },
	{ ACMPB,	C_KCON,	C_HREG,		2, 2 },

	/* adiw, sbiw */
	{ AADDW, C_QCON, C_PREG,		 2, 2 },
	{ ASUBW,	C_QCON,	C_PREG,		2, 2 },

	/* reg byte */
	{ ACOMB,	C_NONE,	C_REG,		 3, 2 },
	{ ADEC,	C_NONE,	C_REG,		 3, 2 },
	{ AASRB,	C_NONE,	C_REG,		 3, 2 },

	{ AADDW, C_REG,	C_REG,		 4, 4 },

	/* reg, reg word */
	{ AMOVW, C_REG,	C_REG,		 1, 2, V2 },
	{ AMOVW, C_REG,	C_REG,		5, 4, V1 },

	{ ABREAK,	C_NONE,	C_NONE,		6, 2 },

	{ ABCLR,	C_BCON,	C_NONE,		7, 2 },

	{ ABLD,	C_BCON,	C_REG,		2, 2 },

	{ AIN,	C_QCON,	C_REG,		2, 2 },

	{ ASBRC,	C_BCON,	C_REG,	2, 2 },

	{ ABR,	C_NONE,	C_SBRA,		8, 2 },
	{ ABR,	C_NONE,	C_LBRA,		9, 4 },
	{ ABR,	C_NONE,	C_ZOREG, 10, 4 },
	{ ABR,	C_NONE,	C_SOREG,	11, 6 },

	{ ABRBS,		C_BCON,	C_CBRA,		12, 2 },
	{ ABRBS,		C_BCON,	C_LBRA,		13, 6 },
	
	{ ABEQ,		C_NONE,	C_CBRA,		14, 2 },
	{ ABEQ,		C_NONE,	C_LBRA,		15, 6 },

	{ ACALL,	C_NONE,	C_SBRA,		8, 2 },
	{ ACALL,	C_NONE,	C_LBRA,		9, 4 },
	{ ACALL,	C_NONE,	C_ZOREG,	10, 4 },
	{ ACALL,	C_NONE,	C_SOREG,	11, 6 },

	{ ARET,		C_NONE,	C_NONE,		16, 2 },

	{ AWORD,	C_LCON,	C_NONE,		17, 2 },

	{ AMOVPMB,	C_ZOZREG,	C_REG,	18, 2 },
	{ AMOVPMB,	C_POSTZREG,	C_REG,	18, 2 },

	{ AMOVB,	C_SOPREG,C_REG,	19, 2 },
	{ AMOVB,	C_SAUTO,	C_REG,	19, 2 },
	{ AMOVB,	C_SEXT,	C_REG,	21, 4 },
	{ AMOVB,	C_REG,	C_SOPREG,20, 2 },
	{ AMOVB,	C_REG,	C_SAUTO,	20, 2 },
	{ AMOVB,	C_REG,	C_SEXT,	22, 4 },
	
	{ AMOVB,	C_LOREG,	C_REG,	23, 8 },
	{ AMOVB,	C_LAUTO,	C_REG,	23, 8 },
	{ AMOVB,	C_LEXT,	C_REG,	25, 0 },
	{ AMOVB,	C_REG,	C_LOREG,	24, 8 },
	{ AMOVB,	C_REG,	C_LAUTO,	24, 8 },
	{ AMOVB,	C_REG,	C_LEXT,	26, 0 },
	
	/* multiple instructions */
	{ AANDW,	C_REG,	C_REG,		 27, 4 },
	{ AEORW,	C_REG,	C_REG,		27, 4 },
	{ AANDL,	C_REG,	C_REG,		 28, 8 },
	{ AEORL,	C_REG,	C_REG,		28, 8 },
	{ ACOMW,	C_NONE,	C_REG,		 29, 4 },
	{ ACOML,	C_NONE,	C_REG,		30, 8 },
	{ AADDW,	C_LCON,	C_HREG,		34, 4 },
	{ AADDL, C_REG,	C_REG,		 31, 8 },
	{ ASUBW,	C_REG,	C_REG,		32, 4 },
	{ ASUBL,	C_REG,	C_REG,		33, 8 },
	{ ASUBW,	C_LCON,	C_HREG,		34, 4 },
	{ ASUBL,	C_LCON,	C_HREG,		35, 8 },
	{ AMOVW,	C_LCON,	C_HREG,		36, 4 },
	{ AMOVL,	C_REG,	C_REG,		37, 4 },
	{ AMOVL,	C_LCON,	C_HREG,		38, 8 },
	{ ACMPW,	C_REG,	C_REG,		39, 6 },
	{ ACMPW,	C_LCON,	C_HREG,		40, 6 },
	{ AADDW,	C_LCON,	C_REG,		41, 8 },
	{ ANEGW,	C_NONE,	C_HREG,		42, 8 },
	{ ANEGL,	C_NONE,	C_HREG,		43, 16 },
	{ ALSLW,	C_NONE,	C_REG,		44, 4 },
	{ ALSRW,	C_NONE,	C_REG,		45, 4 },
	{ ALSLL,	C_NONE,	C_REG,		46, 8 },
	{ ALSRL,	C_NONE,	C_REG,		47, 8 },

	{ AMOVW,	C_SACON,	C_PREG,	48, 4 },
	{ AMOVW,	C_LACON,	C_HREG,	49, 6 },

	{ ABGT,		C_NONE,	C_BBRA,		50, 4 },
	{ ABGT,		C_NONE,	C_LBRA,		51, 8 },

	{ ASUBB,	C_KCON,	C_REG,		52, 4 },
	{ AANDB,	C_KCON,	C_REG,		 52, 4 },
	{ AMOVB,	C_KCON, C_REG,		 52, 4 },
	{ ACBRB,	C_KCON,	C_REG,		52, 4 },
	{ ACMPB,	C_KCON,	C_REG,		52, 4 },

	{ ASUBW,	C_LCON,	C_REG,		53, 8 },

	{ AMULB,	C_HREG,	C_REG,		54, 4 },
	{ AMULB,	C_REG,	C_HREG,		55, 4 },
	{ AMULB,	C_REG,	C_REG,		56, 6 },

	{ APOPW,	C_NONE,	C_REG,		57, 4 },
	{ APOPL,	C_NONE,	C_REG,		58, 8 },

	{ AMOVW,	C_LCON,	C_REG,		59, 6 },
	{ AMOVL,	C_LCON,	C_REG,		60, 12 },
	{ ANEGW,	C_NONE,	C_REG,		61, 10 },
	{ ANEGL,	C_NONE,	C_REG,		62, 18 },
	{ ASUBL,	C_LCON,	C_REG,		63, 16 },
	{ ACMPW,	C_LCON,	C_REG,		64, 10 },

	{ AMOVW,	C_LACON,	C_REG,	65, 10 },
	{ AMOVW,	C_SACON,	C_REG,	66, 8 },

	{ ACALL,	C_NONE,	C_ZREG,	67, 2 },

	{ ACBI,	C_BCON,	C_ICON,		68, 2 },
	{ ASBIC,	C_BCON,	C_ICON,	68, 2 },

	{ AMOVB,	C_SOREG,	C_REG,	69, 4 },
	{ AMOVB,	C_REG,	C_SOREG,	70, 4 },

	{ AMOVW,	C_ZOREG,	C_REG,	71, 6 },
	{ AMOVW,	C_LOREG,	C_REG,	72, 10 },
	{ AMOVL,	C_ZOREG,	C_REG,	73, 10 },
	{ AMOVL,	C_LOREG,	C_REG,	74, 14 },

	{ AMOVW,	C_REG,	C_ZOREG,	75, 6 },
	{ AMOVW,	C_REG,	C_LOREG,	76, 10 },
	{ AMOVL,	C_REG,	C_ZOREG,	77, 10 },
	{ AMOVL,	C_REG,	C_LOREG,	78, 14 },

	{ AMOVB,	C_PREREG,	C_REG,	79, 2 },
	{ AMOVB,	C_POSTREG,	C_REG,	79, 2 },
	{ AMOVB,	C_REG,	C_PREREG,	80, 2 },
	{ AMOVB,	C_REG,	C_POSTREG,	80, 2 },

	{ ADATA,	C_LCON,	C_NONE,	81, 0 },

	{ AMOVPMB,	C_ZOREG,	C_REG,	82, 4 },
	{ AMOVPMB,	C_LOREG,	C_REG,	83, 8 },
	{ AMOVPMW,	C_ZOREG, C_REG,	84, 6 },
	{ AMOVPMW,	C_LOREG,	C_REG,	85, 10 },
	{ AMOVPML,	C_ZOREG,	C_REG,	86, 10 },
	{ AMOVPML,	C_LOREG,	C_REG,	87, 14 },
	{ AMOVPMB,	C_SOREG, C_REG,	88, 6 },
	{ AMOVPMW,	C_SOREG, C_REG,	89, 8 },
	{ AMOVPML,	C_SOREG, C_REG,	90, 12 },

	{ ABR,	C_NONE,	C_LOREG,	91, 8 },
	{ ACALL,	C_NONE,	C_LOREG,	91, 8 },

	{ AMOVW,	C_SOREG,	C_REG,	92, 6 },
	{ AMOVL,	C_SOREG,	C_REG,	93, 12 },
	{ AMOVW,	C_REG,	C_SOREG,	94, 6 },
	{ AMOVL,	C_REG,	C_SOREG,	95, 12 },

	{ AVECTOR,	C_NONE,	C_SEXT,	96, 4 },
	{ AVECTOR,	C_NONE,	C_LBRA,	9, 4 },

	{ AMOVB,	C_ZOPREG,	C_REG,	97, 2 },
	{ AMOVB,	C_REG,	C_ZOPREG,	98, 2 },

	{ AADDB,	C_LCON,	C_HREG,		99, 2 },
	{ AADDB,	C_LCON,	C_REG,		100, 4 },

	{ AMOVPMB,	C_SEXT,	C_REG,	101, 6 },
	{ AMOVPMW,	C_SEXT,	C_REG,	102, 8 },
	{ AMOVPML,	C_SEXT,	C_REG,	103, 12 },

	{ ASAVE,	C_NONE,	C_NONE,	104, 0 },
	{ ARESTORE,	C_NONE,	C_NONE,	105, 0 },

	{ AXXX,	C_NONE,	C_NONE,		0, 0 },
};
