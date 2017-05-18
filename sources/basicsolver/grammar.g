%start parser, problem;
%token	VARSTOK, DOMAINSTOK, CONSTRAINTSTOK, SOLVETOK, ALLTOK, 
	INTTYPE, BOOLTYPE, ARROWTOK,
	BRACKOPEN, BRACKCLOSE, MINUSTOK, PLUSTOK, STARTOK, POWTOK, COMMATOK,
	SEMITOK, COLONTOK, ISTOK, SMALLERTOK, GREATERTOK, NEQTOK, LEQTOK, GEQTOK, MODTOK, 
	DIVTOK, PARENTOPEN, PARENTCLOSE, VARTOK, DECTOK, RANGETOK, MAXTOK, MINTOK, ABSTOK, ANYTOK;
%options "thread-safe generate-lexer-wrapper generate-symbol-table";
%datatype "Problem", "problem.h";
%lexical yylex;
%top{
#include "problem.h"
}
{
	#include <stdio.h>
	#include <stdlib.h>
	#include <assert.h>
	#include "datatypes.h"
	#include "problem.h"
	#include "variable.h"
	#include "constraint.h"
	#include "solve.h"
	
	extern char * yytext;	
	extern int lineNr;
	void LLmessage(struct LLthis *llthis, int token) {
		printf("Parse error: line %d, unexpected token %s\n", lineNr, yytext);
		exit(EXIT_FAILURE);
	}
}
problem<Problem>	:	
	body(&LLretval)
;

body(Problem *p) :	
	vars<v> { 
		*p = emptyProblem(); 
		setVarsOfProblem(*p, v);
		
	}
	domains(*p)
	constraints<c> {
		setConstraintsOfProblem(*p, c); 
	}
	solvespec<s> { 
		(*p)->solvespec = s;
	}
;

vars<VarList>{ LLretval = NULL; } : 
	VARSTOK COLONTOK 
	[	
		varlist<l> 
		COLONTOK 
		datatype<t>	{ LLretval = addVars(LLretval, t, l);} 
		SEMITOK
	]*
;	
	
	
datatype<DataType>	: 
	INTTYPE	{ 
		LLretval = INTEGER;
	} 
	|	
	BOOLTYPE { 
		LLretval = BOOLEAN;
	}
;
	
	
domains(Problem p)	:
	DOMAINSTOK COLONTOK [domainspec(p) SEMITOK]*
;	
	
	
domain<IntegerSet>	{int min = 0;} : 
	[
		BRACKOPEN 
		| 
		PARENTOPEN	{
			min = 1;
		}
	] 
	subdomain<sd> { 
		LLretval = emptyIntegerSet();
		sd.min += min; 
		addIntervalToSet(LLretval, sd.min, sd.max);
	}
	[	
		COMMATOK 
		subdomain<sd>	{
			addIntervalToSet(LLretval, sd.min, sd.max); 
		}
	]* 
	[
		BRACKCLOSE 
		|
		PARENTCLOSE {
			removeIntegerFromSet(LLretval, sd.max); 
		}
	]	
;



subdomain<Tuple> {int max;} :
	negposint<min> {
		max = min; 
	} 
	[RANGETOK 
	negposint<max>
	]? 
	{LLretval = (Tuple){min, max};}
;
		
integer<int> :
	DECTOK	{ 
		LLretval = atoi(yytext); 
	}
;	

negposint<int> {int mult = 1;}:
	[MINUSTOK {mult=-1;}]?
	DECTOK	{ 
		LLretval = mult*atoi(yytext); 
	}
;
	
constraints<ConstraintList>	: 
	CONSTRAINTSTOK 
	COLONTOK {
		LLretval = NULL; 
	}
	[	
		constraint<c> {
			LLretval = addConstraint(LLretval, c); 
		}
		SEMITOK
	]*
;
		
constraintlist<ConstraintList> : 
	constraint<c> { 
		LLretval = addConstraint(NULL, c); 
	}
	[	
		COMMATOK
		constraint<c> { 
			LLretval = addConstraint(LLretval, c); 
		}
	]*
;	
	
solvespec<SolveSpec> : 
	SOLVETOK COLONTOK 
	[
		ALLTOK { 
			LLretval.type = SOLVEALL;
		} 
		| 
		integer<max> { 
			LLretval.type = SOLVENR; 
			LLretval.max = max; 
		}
	] 
;	
	
var<int> : 
	VARTOK integer<LLretval>
;
	
functioncall<FunctionCall>	{ int argc; FunctionName name; void *argv; } :	
	[	
		[MAXTOK {name = MAX;} | MINTOK {name = MIN;}]
		PARENTOPEN numExp<arg1> COMMATOK numExp<arg2> 
		PARENTCLOSE {	
			argc = 2; 
			NumExp *ar = safeMalloc(argc*sizeof(NumExp));
			ar[0] = arg1;
			ar[1] = arg2;
			argv = ar;
		}
	|	
		ABSTOK 
		PARENTOPEN 
		numExp<arg1> 
		PARENTCLOSE {
			name = ABS; 
			argc = 1;
			NumExp *ar = safeMalloc(argc*sizeof(NumExp));
			ar[0] = arg1;
			argv = ar;
		}
	|	
		[ALLTOK {name = ALL;} | ANYTOK	{name = ANY;}] 
		PARENTOPEN constraintlist<l> 
		PARENTCLOSE {
			argc = 1;
			ConstraintList *p;
			p = safeMalloc(argc*sizeof(ConstraintList));
			p[0] = l;
			argv = p;
		}
	] { LLretval = newFunctionCall(name,argc,argv); }
;
	
varlist<VarList> : 
	var<v>	{LLretval = newVarList(v, NULL);}
	[ COMMATOK var<v2> {addVar(LLretval, v2);} ]* 
;	
	
domainspec(Problem p)	: 
	varlist<vl> ARROWTOK domain<d> {
		setDomainsOfVars(p, vl, d); 
		freeVarList(vl); 
		freeIntegerSet(d);
	}
;	
		
constraint<Constraint>	: 
	numExp<e1>	{LLretval = newValConstraint(e1);}
	[	
		relop<op>	
		numExp<e2>	{ 
			setOperatorOfConstraint(LLretval, op); 
			setSecondExp(LLretval, e2);
		}
	]?	
;	
	
numExp<NumExp>	:	
	term<t1> {LLretval = newNumExp(t1);}	
	[termop<op> term<t2> {LLretval = addTerm(LLretval, op, t2);}]*
;
	
term<Term>	: 
	factor<f> {LLretval = newTerm(f);}
	[factorop<o> factor<f2>	{ LLretval = addFactor(LLretval, o, f2); }]*
;
	
factor<Factor>	:	
	value<v> { LLretval = newValueFactor(v); }
	|	
	MINUSTOK factor<f>	{ LLretval = newMinusFactor(f); }
;
	
value<Value>	: 
	[
		integer<i>	{ LLretval = newIntVal(i); } 
		| 	
		var<id>	{ LLretval = newVarVal(id); } 
		| 	
		functioncall<fc>{ LLretval = newFuncVal(fc); } 
		| 	
		PARENTOPEN numExp<exp> PARENTCLOSE	{ LLretval = newNumExpVal(exp); } 
	] 
	[POWTOK factor<f> {LLretval->exponent = f;}]? 
;	
	
relop<RelOperator>	: 
	ISTOK	{ LLretval = IS; } 
	| 
	SMALLERTOK { LLretval = SMALLER; } 
	| 
	GREATERTOK { LLretval = GREATER; } 
	| 
	NEQTOK { LLretval = NEQ; } 
	| 
	LEQTOK { LLretval = LEQ; } 
	| 
	GEQTOK { LLretval = GEQ; } 
;
	
termop<TermOperator> 	: 
	MINUSTOK {LLretval = MINUS;}
	|	
	PLUSTOK	{LLretval = PLUS;}
;
	
factorop<FactorOperator>	: 
	STARTOK	{ LLretval = MUL; }
	| 
	MODTOK	{ LLretval = MOD; }
	| 
	DIVTOK	{ LLretval = DIV; }
;

