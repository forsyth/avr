%{
#include <u.h>
#include <libc.h>
#include <bio.h>
#include "../zc/z.out.h"
#include "a.h"
%}
%union
{
	Sym	*sym;
	long	lval;
	double	dval;
	char	sval[8];
	Gen	gen;
}
%left	'|'
%left	'^'
%left	'&'
%left	'<' '>'
%left	'+' '-'
%left	'*' '/' '%'
%token	<lval>	LMOVW LMOVB LCOM LEOR LADD LCMP LCBR LBCLR
%token	<lval>	LIN LOUT LSBIC LSBRC
%token	<lval>	LBRA LBRAS LBST LSBI
%token	<lval>	LNOP LEND LRETT LWORD LTEXT LDATA LRETRN
%token	<lval>	LCONST LSP LSB LFP LPC
%token	<lval>	LREG LR
%token	<lval>	LSPREG
%token	<lval>	LSCHED
%token	<dval>	LFCONST
%token	<sval>	LSCONST
%token	<sym>	LNAME LLAB LVAR
%type	<lval>	con expr pointer offset sreg
%type	<gen>	addr rreg regaddr name zreg
%type	<gen>	cbit imm ximm rel
%%
prog:
|	prog line

line:
	LLAB ':'
	{
		if($1->value != pc)
			yyerror("redeclaration of %s", $1->name);
		$1->value = pc;
	}
	line
|	LNAME ':'
	{
		$1->type = LLAB;
		$1->value = pc;
	}
	line
|	LNAME '=' expr ';'
	{
		$1->type = LVAR;
		$1->value = $3;
	}
|	LVAR '=' expr ';'
	{
		if($1->value != $3)
			yyerror("redeclaration of %s", $1->name);
		$1->value = $3;
	}
|	LSCHED ';'
	{
		nosched = $1;
	}
|	';'
|	inst ';'
|	error ';'

inst:
/*
 * load ints and bytes
 */
	LMOVW rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW regaddr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB addr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB regaddr ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * store ints and bytes
 */
|	LMOVW rreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW rreg ',' regaddr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB rreg ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVB rreg ',' regaddr
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * operations with literal and reg
 * operations with reg/literal and reg
 * operations with reg and reg
 * unary operations
 */
|	LCBR imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LADD rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LADD imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LEOR rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LCOM rreg
	{
		outcode($1, &nullgen, NREG, &$2);
	}
/*
 * move immediate: macro for various combinations
 */
|	LMOVW imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LMOVW ximm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * bit set/load in/from TREG
 */
|	LBST cbit ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * flag bit set/clear
 */
|	LBCLR cbit
	{
		outcode($1, &nullgen, NREG, &$2);
	}
|	LBCLR ',' cbit
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * IO
 */
|	LIN imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LOUT rreg ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * set bit in IO register
 */
|	LSBI cbit ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * branch, branch conditional
 * branch conditional register
 * branch conditional to count register
 */
|	LBRA rel
	{
		outcode($1, &nullgen, NREG, &$2);
	}
|	LBRA addr
	{
		outcode($1, &nullgen, NREG, &$2);
	}
|	LBRA '(' zreg ')'
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LBRA ',' rel
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LBRA ',' addr
	{
		outcode($1, &nullgen, NREG, &$3);
	}
|	LBRA ',' '(' zreg ')'
	{
		outcode($1, &nullgen, NREG, &$4);
	}
|	LBRAS cbit ',' rel
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LBRAS cbit ',' addr
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LBRAS cbit ',' '(' zreg ')'
	{
		outcode($1, &$2, NREG, &$5);
	}
/*
 * skip if bit in IO reg is set/cleared
 */
|	LSBIC cbit ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LSBRC cbit ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * CMP (might need to reconsider order)
 */
|	LCMP rreg ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LCMP imm ',' rreg
	{
		outcode($1, &$2, NREG, &$4);
	}
/*
 * NOP
 */
|	LNOP comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
|	LNOP rreg comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LNOP ',' rreg
	{
		outcode($1, &nullgen, NREG, &$3);
	}
/*
 * word
 */
|	LWORD imm comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
|	LWORD ximm comma
	{
		outcode($1, &$2, NREG, &nullgen);
	}
/*
 * END
 */
|	LEND comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}
/*
 * TEXT/GLOBL
 */
|	LTEXT name ',' imm
	{
		outcode($1, &$2, NREG, &$4);
	}
|	LTEXT name ',' con ',' imm
	{
		outcode($1, &$2, $4, &$6);
	}
|	LTEXT name ',' imm ':' imm
	{
		outgcode($1, &$2, NREG, &$6, &$4);
	}
|	LTEXT name ',' con ',' imm ':' imm
	{
		outgcode($1, &$2, $4, &$8, &$6);
	}
/*
 * DATA
 */
|	LDATA name '/' con ',' imm
	{
		outcode($1, &$2, $4, &$6);
	}
|	LDATA name '/' con ',' ximm
	{
		outcode($1, &$2, $4, &$6);
	}
/*
 * RETURN
 */
|	LRETRN	comma
	{
		outcode($1, &nullgen, NREG, &nullgen);
	}

rel:
	con '(' LPC ')'
	{
		$$ = nullgen;
		$$.type = D_BRANCH;
		$$.offset = $1 + pc;
	}
|	LNAME offset
	{
		$$ = nullgen;
		if(pass == 2)
			yyerror("undefined label: %s", $1->name);
		$$.type = D_BRANCH;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LLAB offset
	{
		$$ = nullgen;
		$$.type = D_BRANCH;
		$$.sym = $1;
		$$.offset = $1->value + $2;
	}

rreg:
	sreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
	}

zreg:
	sreg
	{
		$$ = nullgen;
		$$.type = D_REG;
		$$.reg = $1;
		if($1 != D_Z)
			yyerror("indirect jump must use Z reg");
	}


cbit:	con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $1;
	}

ximm:
	'$' addr
	{
		$$ = $2;
		$$.type = D_CONST;
	}
|	'$' LSCONST
	{
		$$ = nullgen;
		$$.type = D_SCONST;
		memcpy($$.sval, $2, sizeof($$.sval));
	}

imm:	'$' con
	{
		$$ = nullgen;
		$$.type = D_CONST;
		$$.offset = $2;
	}

sreg:
	LREG
|	LR '(' con ')'
	{
		if($$ < 0 || $$ >= NREG)
			print("register value out of range\n");
		$$ = $3;
	}

regaddr:
	'(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $2;
		$$.offset = 0;
	}
|	'(' sreg ')' '+'
	{
		$$ = nullgen;
		$$.type = D_POSTREG;
		$$.reg = $2;
		if(!xyzreg($2))
			yyerror("post-increment on invalid register");
	}
|	'-' '(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_PREREG;
		$$.reg = $3;
		if(!xyzreg($3))
			yyerror("pre-increment on invalid register");
	}

addr:
	name
|	con '(' sreg ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.reg = $3;
		$$.offset = $1;
	}

name:
	con '(' pointer ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = $3;
		$$.sym = S;
		$$.offset = $1;
	}
|	LNAME offset '(' pointer ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = $4;
		$$.sym = $1;
		$$.offset = $2;
	}
|	LNAME '<' '>' offset '(' LSB ')'
	{
		$$ = nullgen;
		$$.type = D_OREG;
		$$.name = D_STATIC;
		$$.sym = $1;
		$$.offset = $4;
	}

comma:
|	','

offset:
	{
		$$ = 0;
	}
|	'+' con
	{
		$$ = $2;
	}
|	'-' con
	{
		$$ = -$2;
	}

pointer:
	LSB
|	LSP
|	LFP

con:
	LCONST
|	LVAR
	{
		$$ = $1->value;
	}
|	'-' con
	{
		$$ = -$2;
	}
|	'+' con
	{
		$$ = $2;
	}
|	'~' con
	{
		$$ = ~$2;
	}
|	'(' expr ')'
	{
		$$ = $2;
	}

expr:
	con
|	expr '+' expr
	{
		$$ = $1 + $3;
	}
|	expr '-' expr
	{
		$$ = $1 - $3;
	}
|	expr '*' expr
	{
		$$ = $1 * $3;
	}
|	expr '/' expr
	{
		$$ = $1 / $3;
	}
|	expr '%' expr
	{
		$$ = $1 % $3;
	}
|	expr '<' '<' expr
	{
		$$ = $1 << $4;
	}
|	expr '>' '>' expr
	{
		$$ = $1 >> $4;
	}
|	expr '&' expr
	{
		$$ = $1 & $3;
	}
|	expr '^' expr
	{
		$$ = $1 ^ $3;
	}
|	expr '|' expr
	{
		$$ = $1 | $3;
	}
