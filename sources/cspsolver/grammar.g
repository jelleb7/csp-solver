%start parser, problem;
%token 	VARSTOK, DOMAINSTOK, CONSTRAINTSTOK, SOLVETOK, ALLTOK, 
		INTTYPE, BOOLTYPE, ARROWTOK,
		BRACKOPEN, BRACKCLOSE, MINUSTOK, PLUSTOK, STARTOK, POWTOK, COMMATOK,
		SEMITOK, COLONTOK, ISTOK, SMALLERTOK, GREATERTOK, NEQTOK, LEQTOK, GEQTOK, MODTOK, 
		DIVTOK, PARENTOPEN, PARENTCLOSE, VARNAME, DECTOK, RANGETOK, MAXTOK, MINTOK, ABSTOK, ANYTOK,
		FORALLTOK, ALLDIFFTOK, INTOK, ENDTOK, SUMTOK, PRODUCTTOK, MAXIMUMTOK, MINIMUMTOK, EQUALTOK, INCREASINGTOK, DECREASINGTOK;
%options "generate-lexer-wrapper";
%lexical yylex;

%top{
#include "vardb.h"
#include "rewrite.h"
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
}

{
	#include <stdio.h>
	#include <stdlib.h>
	
	extern char * yytext; 	
	extern int lineNr;
	int solver_pid;

	void LLmessage(int token) {
		fprintf(stderr, "[PARSE ERROR] [line %d] unexpected token '%s'\n", lineNr, yytext);
		exit(EXIT_FAILURE);
	}
	
	char *copyToken() {
		int len = strlen(yytext);
		char *token = malloc(len+1);
		strcpy(token, yytext);
		return token;
	}
	
	void prnt(char *str) {
		fprintf(stdout, "%s", str);
	}
	
	void dimLoop(int *dimsizes, int dim, int maxdim, int printCnt, FILE *inp) {
		if(dim == maxdim || dim == maxdim-1) {
			int val;
			fprintf(stdout, "\t");
			for(int i = 0; i < printCnt; i++) {
				fscanf(inp, "%d", &val);
				fprintf(stdout, "%d ", val);
			}
			fprintf(stdout, "\n");
		} else {
			int dimsize = dimsizes[dim];
			for(int i = 0; i < dimsize; i++) {
				dimLoop(dimsizes, dim+1, maxdim, printCnt/dimsize, inp);
			}
		}
	}
	
	void printSolution(FILE *inp) {
		List variables = getAllVariables();
		while(variables != NULL) {
			Variable var = variables->item;
			fprintf(stdout, "%s = ", var->name);
			List dimsizes = var->dimsizes;
			int *dimsizes2 = safeMalloc(var->dims*sizeof(int));
			int cnt = 1;
			int dimsize;
			for(int i = 0; i < var->dims; i++) {
				dimsize = calcNumExp(dimsizes->item, NULL);
				dimsizes2[i] = dimsize;
				cnt *= dimsize;
				dimsizes = dimsizes->next;
			}
			dimLoop(dimsizes2, 0, var->dims, cnt, inp);
			variables = variables->next;
		}
	}
	
	void exitChild() {
		exit(0);
	}
	
	void waitForChild() {
		int returnStatus; 
		waitpid(solver_pid, &returnStatus, 0);
		if (returnStatus == 1) {
		   printf("The child process terminated with an error!.");
		}
	}
	
	int match(char *str1, char *str2) {
		return (strcmp(str1, str2) == 0);
	}
	
	void exitParent() {
		kill(solver_pid, SIGKILL);
		waitForChild();
		exit(0);
	}
	
	
	
	int main(int argc, char** argv) {
		FILE *solution_file;	

		createEmptyVarDB();
		currentIndex = 0;
		
		if(argc < 2) {
			fprintf(stderr, "[ERROR] No input file given\n"); 
			exit(-1);		
		}
		
		if(match(argv[1], "-h") || match(argv[1], "-help")) {
			printf("Usage: ./csp [OPTION]... [FILE]\n");
			printf("Solve the CSP problem defined in FILE using the given options.\n\n");
			printf("-h, -help    Display this help and exit.\n");
			printf("-iconst 1    Make the problem initially node-consistent.\n");
			printf("-iconst 2    Make the problem initially arc-consistent.\n");
			printf("-mrv         Use the heuristic Minimum Remaining Values (MRV).\n");
			printf("-deg         Use the degree heuristic.\n");
			printf("-arc         Keep the problem arc-consistent. Can not be combined with '-fc'\n");
			printf("-fc          Use Forward Checking. Can not be combined with '-arc'.\n");
			printf("\n");
			exit(0);
		}

		stdin = fopen(argv[argc-1], "r");	
		if(stdin == NULL) {
			fprintf(stderr, "[ERROR] Could not open %s\n", argv[argc-1]);
			exit(-1);
		}		
		stdout = fopen("translation.csp", "w");	
		
  		parser();
  		freopen("/dev/stdin", "r", stdin);
  		freopen("/dev/stdout", "w", stdout);
  		
  		solver_pid = fork();
		if(solver_pid == -1) {
		    fprintf (stderr, "[ERROR] Could not fork\n");
		    exit(EXIT_FAILURE);
		}
  		
  		if(solver_pid == 0) {		    
		    signal(SIGKILL, exitChild);
		    
		    argv[argc-1] = "translation.csp";
		    int error;
		    if( access("./solver", F_OK ) != -1 ) { // check if file exists
		    	argv[0] = "./solver";
		    	error = execvp("./solver", argv);
		    } else {
		    	argv[0] = "solver";
		    	error = execvp("solver", argv);
		    }
			fprintf(stderr, "[ERROR] Solver executed with errorcode %d.\n", error);
		} else {
			signal(SIGINT, exitParent);
			waitForChild();
			
			solution_file = fopen("solution.txt", "r");
			if(solution_file == NULL) {
				exit(-1);
			}
			int backtracking_points, solution_count;
		    
		    fscanf(solution_file, "%d", &backtracking_points);
		    fscanf(solution_file, "%d", &solution_count);
		    
		    for(int i = 0; i < solution_count; i++) {
		    	printf("### Solution %d ###\n", i+1);
		    	printSolution(solution_file);
		    	printf("\n");
		    }   
		    printf("Number of visited states: %d\n", backtracking_points);
		    printf("Number of solutions: %d\n\n", solution_count);
		    fclose(solution_file);
		    unlink("solution.txt");
		}
  		
  		
  		  		
        
	  	return 0;
	}

}

problem		:	
				body
			;

body		:	
				vars domains constraints solvespec
			;

vars		:	VARSTOK COLONTOK {prnt("variables:\n");} 
				[
					vardeflist 
					COLONTOK {
						prnt(" : ");
					} 
					datatype 
					SEMITOK {
						prnt(";\n");
					}
				]*
				{prnt("\n");}
			;				
			
domains 	:	DOMAINSTOK COLONTOK {prnt("domains:\n");}
				[
					domainspec<domspec> {
						rewriteDomainSpec(domspec, NULL);
					}
				]*
				{checkDomainsSet(); prnt("\n");}
			;	

constraints	:	CONSTRAINTSTOK COLONTOK {prnt("constraints:\n");}
				[
					constraintspec<cspec> {
						rewriteConstraintSpec(cspec, NULL);
					}
				]*
				{prnt("\n");}
			;

solvespec	:	
				SOLVETOK			{prnt(yytext);}
				COLONTOK 			{prnt(yytext);}
				[ALLTOK | posint]	{prnt(yytext);}
				{prnt("\n");}
			;

domainspec<RecursiveType> {Type type; void *data; List items = NULL;}
			:	
				[
					forallspec<fa> 
					[
						domainspec<domspec> 
						{items = addToListEnd(domspec, items);}
					]* 
					ENDTOK {type = FORALL; fa->items = items; data = fa;}
					| 
					varlist<vars> ARROWTOK domain<doms> SEMITOK 
					{
						type = DOMSET; 
						data = newDomainSet(vars, doms);
					}
				]
				{LLretval = newRecursiveType(type, data);}
			;
			
constraintspec<RecursiveType>  {Type type; void *data; List items = NULL;}
			:	[
					forallspec<fa> 
					[
						constraintspec<cspec>
						{items = addToListEnd(cspec, items);}
					]* 
					ENDTOK {type = FORALL; fa->items = items; data = fa;}
					| 
					constraint<c> SEMITOK 
					{
						type = CONSTRAINT;
						data = c;
					}
				]
				{LLretval = newRecursiveType(type, data);}
			;

forallspec<ForAll> 
			:	
				FORALLTOK PARENTOPEN varname<name> INTOK domain<values> PARENTCLOSE
				{LLretval = newForAll(name, values, NULL);}
			;

datatype	:	
				[INTTYPE | BOOLTYPE]
				{prnt(yytext);}
			;


domain<List> {LLretval = NULL;}
			:	
				BRACKOPEN
				subdomain<dom> {LLretval = addToListEnd(dom, LLretval);}
				[	
					COMMATOK 
					subdomain<dom> {LLretval = addToListEnd(dom, LLretval);}
				]* 
				BRACKCLOSE
			;
	
subdomain<Subdomain> {NumExp min, max;}
			:	
				numexp<min> {max = min;}
				[
					RANGETOK 
					numexp<max>
				]?
				{LLretval = newSubdomain(min, max);}
			;

posint<int>	:	
				DECTOK {LLretval = atoi(yytext);}
			;

constraint<Constraint> {NumExp exp2 = NULL; NumExp exp1 = NULL; char *op = NULL;}
			:	
				numexp<exp1>
				[
					relop {op = copyToken();}
					numexp<exp2>
				]?
				{LLretval = newConstraint(exp1, op, exp2);}
			;

numexp<NumExp> {List termList = NULL; List opList = NULL;}
		 	:	
		 		term<t> {termList = addToListEnd(t, termList);}
		 		[
		 			termop {opList = addToListEnd(copyToken(), opList);}
		 			term<t> {termList = addToListEnd(t, termList);}
		 		]*
		 		{LLretval = newNumExp(termList, opList);}
		 	;
		 	
term<Term>	{List factorList = NULL; List opList = NULL; } 
			:
				factor<f> {factorList = addToListEnd(f, factorList);} 
				[
					factorop {opList = addToListEnd(copyToken(), opList);} 
					factor<f> {factorList = addToListEnd(f, factorList);} 
				]*
				{LLretval = newTerm(factorList, opList);} 
			;
		
factor<Factor> {Type ftype = -1; void *data;}
			:	
				[
					value<v> {ftype = VALUE; data = v;}
					| 
					MINUSTOK 
					factor<f> {ftype = NEGATION; data = f;}
				]
				{LLretval = newFactor(ftype, data);}
			;

value<Value>	{Type vtype = -1; int powtok = 0; Factor powFactor = NULL; void *data; }
			:	
				[
					posint {vtype = INT; data = copyToken();}
					| 
					varcall<vsc>	{vtype = VARSUBSTCALL; data = vsc;}
					| 
					functioncall<fc>	{vtype = FUNCTIONCALL; data = fc;}
					| 
					PARENTOPEN
					numexp<e> 	{vtype = NUMEXP; data = e;}
					PARENTCLOSE
				] 
				[	
					POWTOK {powtok = 1;}
					factor<powFactor>
				]?
				{LLretval = newValue(vtype, data, powtok, powFactor);}
			;
					
					
constraintlist<List>	{LLretval = NULL;}
			:	
				constraint<c> {LLretval = addToListEnd(c, LLretval);}
				[COMMATOK constraint<c2> {LLretval = addToListEnd(c2, LLretval);}]*
			;

varname<char *>	:	
	VARNAME {LLretval = copyToken();}
;

indexspec<NumExp>	:	
	BRACKOPEN numexp<LLretval> BRACKCLOSE
;

vardef(int comma) {int dims = 0; List dimsizes = NULL;}	:
	varname<name>
	[
		indexspec<idx> {
			dims++; 
			dimsizes = addToListEnd(idx, dimsizes);
		}
	]*
	{
		Variable var = newVariable(name, dims, dimsizes);
		addVarToDB(var);
		rewriteVarDef(var, comma);
	}
;

varcall<VarCall> {List indices = NULL; char *name;}:	
	varname<name>
	[
		domain<dom> {indices = addToListEnd(dom, indices); }
	]*
	{LLretval = newVarCall(name, indices);}
;

functioncall<FunctionCall> {char *funcName; Type type = -1; List argList = NULL;}	
				:	[
						[MAXTOK | MINTOK] {funcName = copyToken(); type = MAXMIN;}
						PARENTOPEN
						numexp<exp1> {argList = addToListEnd(exp1, argList);}
						COMMATOK
						numexp<exp2> {argList = addToListEnd(exp2, argList);}
						PARENTCLOSE
						| 
						[ALLTOK | ANYTOK] {funcName = copyToken(); type = ALLANY;}
						PARENTOPEN 
						constraintlist<cl> {argList = cl;}
						PARENTCLOSE 
						|
						ABSTOK {funcName = copyToken(); type = ABS;}
						PARENTOPEN
						numexp<e> {argList = addToListEnd(e, argList);}
						PARENTCLOSE
						|
						[
							ALLDIFFTOK {funcName = copyToken(); type = ALLDIFF;}
							|
							SUMTOK {funcName = copyToken(); type = SUM;}
							|
							PRODUCTTOK {funcName = copyToken(); type = PRODUCT;}
							|
							MAXIMUMTOK {funcName = copyToken(); type = MAXIMUM;}
							|
							MINIMUMTOK {funcName = copyToken(); type = MINIMUM;}
							|
							INCREASINGTOK {funcName = copyToken(); type = INCREASING;}
							|
							DECREASINGTOK {funcName = copyToken(); type = DECREASING;}
							|
							EQUALTOK {funcName = copyToken(); type = EQUAL;}
						]
						PARENTOPEN 
						varcall<vc> {argList = addToListEnd(vc, argList);}
						[COMMATOK varcall<vc> {argList = addToListEnd(vc, argList);}]*
						PARENTCLOSE
					]
					{LLretval = newFunctionCall(type, funcName, argList);}
		 		;
		 		
varlist<List>	{LLretval = NULL;}
				:	
					varcall<vc> {LLretval = addToListEnd(vc, LLretval);} 
					[
						COMMATOK varcall<vc> {LLretval = addToListEnd(vc, LLretval);} 
					]*
				;

vardeflist      :	vardef(0) [COMMATOK vardef(1)]*;





relop			:	
					ISTOK | SMALLERTOK | GREATERTOK | NEQTOK | LEQTOK | GEQTOK
				;
				
termop			:	
					MINUSTOK | PLUSTOK
				;
				
factorop		:	
					STARTOK | MODTOK | DIVTOK 
				;


