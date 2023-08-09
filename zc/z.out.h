#define	NSYM	50
#define	NSNAME	8

#define	REGMUL	0	/* 0-1 */
#define	REGZERO	2	/* 2-3 */
#define	REGRETL	4	/* 4-7 */
#define	REGFIRST	8	/* 8-19 general registers */
#define	REGRET	20	/* 20-21 */
#define	REGTMP	22	/* 22-23 */
#define	REGW	24	/* 24-25 */
#define	REGX	26	/* 26-27 */
#define	REGY	28	/* 28-29 */
#define	REGZ	30	/* 30-31 */
#define	NREG	32

#define	SREG		0x3f

#define	BIT(b)	(1<<(b))
#define	BIT2(b)	(3<<(b))
#define	BIT4(b)	(15<<(b))
#define	BITR(a, b)	(((1<<(b-a))-1)<<a)

#define	REGLAST	REGRET
#define	REGARG	REGRET
#define	REGARGL	REGRETL
#define	REGHI	16
#define	REGSP	REGY
#define	REGEXT	0	/* not used */
#define	REGMASK	(BITR(REGFIRST, REGLAST)|BIT2(REGW)|BIT2(REGX))

#define	FREGRET	0
#define	FREGEXT	0
#define	NFREG	0

enum	as
{
	AXXX = 0,

	AADDB,	/* ADD */
	AADDBC,	/* ADC */
	AADDL,
	AADDW,	/* ADIW, macro add word */
	AANDB,	/* includes ANDI */
	AANDL,
	AANDW,
	AASRB,
	AASRL,
	AASRW,
	ABCLR,
	ABLD,
	ABSET,
	ABST,
	ACALL,	/* includes RCALL, ICALL, EICALL, and CALL */
	ACBI,
	ACBRB,
	ACBRL,
	ACBRW,
	ACLRB,
	ACLRL,
	ACLRW,
	ACMPB,		/* includes CPI */
	ACMPBC,
	ACMPW,
	ACMPL,
	ACOMB,
	ACOML,
	ACOMW,
	ACMPSE,
	ADEC,
	AEORB,
	AEORL,
	AEORW,
	AFMULSB,
	AFMULSUB,
	AFMULUB,
	AIN,
	AINC,
	ALSLB,
	ALSLL,
	ALSLW,
	ALSRB,
	ALSRL,
	ALSRW,
	AMOVB,	/* MOV, LD, LDD, LDI, LDS, ST, STD, STS */
	AMOVBL,
	AMOVBW,
	AMOVBZL,
	AMOVBZW,
	AMOVL,
	AMOVPM,	/* LPM, ELPM */
	AMOVPMB,
	AMOVPML,
	AMOVPMW,
	AMOVW,
	AMOVWL,
	AMOVWZL,
	AMULB,
	AMULUB,
	AMULSUB,
	ANEGB,
	ANEGL,
	ANEGW,
	AORB,	/* includes ORI */
	AORL,
	AORW,
	AOUT,
	APOPB,
	APOPL,
	APOPW,
	APUSHB,
	APUSHL,
	APUSHW,
	ARET,
	ARETI,
	AROLB,
	AROLL,
	AROLW,
	ARORB,
	ARORL,
	ARORW,
	ASBI,
	ASBIC,
	ASBIS,
	ASBRC,
	ASBRS,
	ASBRB,
	ASBRL,
	ASBRW,
	ASUBB,	/* includes SUBI */
	ASUBBC,	/* includes SBCI */
	ASUBL,
	ASUBW,	/* includes SBIW */
	ASWAP,
	ATST,

	/* BRBS, BRBC: */
	ABRBS,
	ABRBC,

	/* C (carry), Z (zero), N (negative), V (overflow), S (N ^ V), H (half carry), T (transfer), I (interrupt) */

	ABR,

	ABEQ,
	ABNE,
	ABCS,
	ABCC,
	ABSH,	/* same or higher */
	ABSL,	/* same or lower */
	ABLO,
	ABMI,
	ABPL,
	ABGE,
	ABLT,
	ABHC,
	ABHS,
	ABTC,
	ABTS,
	ABVC,
	ABVS,
	ABID,
	ABIE,
	ABGT,
	ABHI,
	ABLE,

	/* SEC etc are done using #define to BSET/BCLR */

	ABREAK,
	ANOP,
	ASLEEP,
	AWDR,

	ANOOP,
	ADATA,
	AGLOBL,
	AGOK,
	AHISTORY,
	ANAME,
	ASIGNAME,
	ATEXT,
	AWORD,
	AEND,
	ADYNT,
	AINIT,
	APM,
	AVECTOR,
	ASAVE,
	ARESTORE,
	ALAST
};

enum
{
	NBREG		= 32,
	NWREG		= NBREG/2,
	NLREG		= NBREG/4,

	D_R0		= 0,
	D_W			= D_R0+24,		/* 25:24 */
	D_X			= D_R0+26,		/* 27:26 */
	D_Y			= D_R0+28,		/* 29:28 */
	D_Z			= D_R0+30,		/* 31:30 */
	D_NONE		= D_R0+NBREG,

/* name */
	D_EXTERN,
	D_STATIC,
	D_AUTO,
	D_PARAM,

/* type */
	D_BRANCH,
	D_OREG,
	D_CONST,
	D_FCONST,
	D_SCONST,
	D_REG,
	D_FREG,
	D_SREG,
	D_PORT,

	D_RAMPD,
	D_RAMPY,
	D_RAMPZ,

	D_PREREG,
	D_POSTREG,

	/*D_QUICK,*/

	D_FILE,
	D_FILE1,
};

/*
 * this is the ranlib header
 */
#define	SYMDEF	"__.SYMDEF"

/*
 * this is the simulated IEEE floating point
 */
typedef	struct	ieee	Ieee;
struct	ieee
{
	long	l;	/* contains ls-man	0xffffffff */
	long	h;	/* contains sign	0x80000000
				    exp		0x7ff00000
				    ms-man	0x000fffff */
};
