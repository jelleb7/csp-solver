#include "rewrite.h"
#include "vardb.h"
#include <math.h>

#define DEBUG 0

extern int lineNr;

void error(const char *str) {
	fprintf(stderr, "[ERROR] ");
	fprintf(stderr, "%s", str);
	fprintf(stderr, "\n");
	exit(-1);
}

void debug(const char *line) {
	if(DEBUG) {
		fprintf(stdout, "[DEBUG] "); 
		fprintf(stdout, "%s", line);
	}
}

void printNFVar(int idx) {
	fprintf(stdout, "X%d", idx);
}

void printComma(int boolean) {
	if(boolean) {
		fprintf(stdout, ", ");
	}
}

void printBracketOpen() {
	fprintf(stdout, "[");
}

void printBracketClose() {
	fprintf(stdout, "]");
}

void printLineEnd() {
	fprintf(stdout, ";\n");
}

int min(int val1, int val2) {
	return (val1 < val2 ? val1 : val2);
}


int max(int val1, int val2) {
	return (val1 > val2 ? val1 : val2);
}
	
int abs(int val) {
	return (val < 0 ? val *-1 : val);
}	
	
int getGlobalIndex(Variable var, int subidx) {
	return var->globalIndex + subidx;
}

int getSubstVal(char *varname, List substset) {
	while(substset != NULL) {
		Substitution s = (Substitution) substset->item;
		if(!strcmp(s->var, varname)) {
			return s->val;
		}
		substset = substset->next;
	}
	char str[1000];
	strcpy(str, varname);
	strcat(str, "is not defined.");
	error(str);
	return -1;
}

int dimCount(List dimsizes) {
	int cnt = 1;
	while(dimsizes != NULL) {
		NumExp idx = (NumExp) dimsizes->item;
		cnt *= calcNumExp(idx, NULL);
		dimsizes = dimsizes->next;
	}
	return cnt;
}

Substitution getPossibleSubstitution(char *varname, List substset) {
	while(substset != NULL) {
		Substitution s = substset->item;
		if(!strcmp(varname, s->var)) {
			return s;
		}
		substset = substset->next;
	}
	return NULL;
}

Substitution getSubstitution(char *varname, List substset) {
	Substitution s = getPossibleSubstitution(varname, substset);
	if(s == NULL) {
		char str[1000];
		strcpy(str, varname);
		strcat(str, " is not defined here.\n");
		error(str);
	}
	return s;
}


int calcFunctionCall(FunctionCall fc, List substset) {
	if(fc->type == MAXMIN) {
		int val1 = calcNumExp(fc->argList->item, substset);
		int val2 = calcNumExp(fc->argList->next->item, substset);
		if(!strcmp(fc->funcName, "max")) {
			return max(val1, val2);
		} 
		return min(val1, val2);
	}
	if(fc->type == ABS) {
		return abs(calcNumExp(fc->argList->item, substset));
	}
	if(fc->type == ALLANY) {
		List constraints = fc->argList;
		if(!strcmp(fc->funcName, "all")) {
			while(constraints != NULL) {
				if(!calcConstraint(constraints->item, substset)) {
					return 0;
				}
			}
			return 1;
		} 
		// any
		while(constraints != NULL) {
			if(calcConstraint(constraints->item, substset)) {
				return 1;
			}
		}
		return 0;
	} else if(fc->type == ALLDIFF) {
		error("alldiff not allowed here\n");
	}
	return -1;
}

int calcValue(Value v, List substset) {
	int val;
	VarCall vc;
	
	switch(v->type) {
		case INT:
			val = atoi((char *) v->data);
			break;
		case VARSUBSTCALL:
			vc = (VarCall) v->data;
			Substitution s = getPossibleSubstitution(vc->name, substset);
			if(s == NULL) {
				char str[1000];
				strcpy(str, "The value of variable ");
				strcat(str, vc->name);
				strcat(str, " can not be determined at compile-time.\n");
				error(str);
			}
			val = s->val;
			break;
		case FUNCTIONCALL:
			val = calcFunctionCall(v->data, substset);
			break;
		case NUMEXP:
			val = calcNumExp(v->data, substset);
			break;
		default:
			error("@calcValue, value has wrong type.\n");
	}
	if(v->powtok) {
		val = pow(val, calcFactor(v->exponent, substset));
	}
	return val;
}

int calcFactor(Factor f, List substset) {
	if(f->type == VALUE) {
		return calcValue(f->data, substset);
	}
	return -calcFactor(f->data, substset);
}

int calcTerm(Term t, List substset) {
	List factorList = t->factorList;
	List operatorList = t->operatorList;
	Factor firstFactor = factorList->item;
	factorList = factorList->next;
	int val = calcFactor(firstFactor, substset);
	while(factorList != NULL) {
		char *operator = operatorList->item;
		Factor t = factorList->item;
		int factorval = calcFactor(t, substset);
		if(!strcmp(operator, "*")) {
			val *= factorval;
		} else if(!strcmp(operator, "div")) {
			val /= factorval;
		} else { // modulo
			val = val % factorval;
		}
		factorList = factorList->next;
		operatorList = operatorList->next;
	}
	return val;
}

int calcNumExp(NumExp exp, List substset) {
	List termList = exp->termList;
	List operatorList = exp->operatorList;
	Term firstTerm = termList->item;
	termList = termList->next;
	int val = calcTerm(firstTerm, substset);
	while(termList != NULL) {
		char *operator = operatorList->item;
		Term t = termList->item;
		int termval = calcTerm(t, substset);
		if(!strcmp(operator, "-")) {
			termval *= -1;
		} 
		val += termval;
		termList = termList->next;
		operatorList = operatorList->next;
	}
	return val;
}

int calcConstraint(Constraint c, List substset) {
	if(c->exp2 == NULL) {
		return calcNumExp(c->exp1, substset);
	}
	int val1 = calcNumExp(c->exp1, substset);
	int val2 = calcNumExp(c->exp2, substset);
	char *operator = c->operator;
	if(!strcmp(operator, "<>")) {
		return (val1 != val2);
	}
	if(!strcmp(operator, "=")) {
		return (val1 == val2);
	}
	if(!strcmp(operator, "<")) {
		return (val1 > val2);
	}
	if(!strcmp(operator, ">")) {
		return (val1 > val2);
	}
	if(!strcmp(operator, "<=")) {
		return (val1 <= val2);
	}
	return (val1 >= val2);
}

int calcElemsSize(List dimsizes, List substset) {
	if(dimsizes == NULL) {
		return 1;
	}
	return calcNumExp(dimsizes->item, NULL) * calcElemsSize(dimsizes->next, substset);
}

/*
int calcSubIndex(int dims, List dimsizes, List indexExps, List substset) {
	if(dims == 0) {
		return 0;
	}
	
	int plus = calcSubIndex(dims-1, dimsizes->next, indexExps->next, substset);
	NumExp idxExp = (NumExp) indexExps->item;
	int dimsize = calcNumExp(dimsizes->item, NULL);
	int subidx;
	subidx = calcNumExp(idxExp, substset);
	if(subidx >= dimsize) {
		error("Index out of bounds.\n"); 
	}
	subidx *= dimCount(dimsizes->next);
	return subidx + plus;
}
*/

void rewriteVarDef(Variable var, int comma) {
	printComma(comma);
	int dims = var->dims;
	
	if(dims == 0) {
		int idx = var->globalIndex;
		printNFVar(idx);
	} else { /* dims > 0 */
		int i;
		int count = 1;
		List dimsizes = var->dimsizes;
		while(dimsizes != NULL) {
			count *= calcNumExp(dimsizes->item, NULL);
			dimsizes = dimsizes->next;
		}
		for(i = 0; i < count; i++) {
			printComma(i);
			int idx = getGlobalIndex(var, i);
			printNFVar(idx);
		}
		
	}
}

/*
void rewriteVarDefList(List vl) {
	rewriteVarDef(vl->item, 1);
	vl = vl->next;
	while(vl != NULL) {
		VarDef vd = vl->item;
		rewriteVarDef(vd, 0);
		vl = vl->next;
	}
}*/

sizedArray obtainVarCallIndices(VarCall vc, List substset) {
	char *varname = vc->name;
	Variable var = getVariableFromDB(varname);
	if(var == NULL) {
		char errorstr[100];
		strcpy(errorstr, "Variable ");
		strcat(errorstr, varname);
		strcat(errorstr, " is not declared.\n");
		error(errorstr);
	}
	int dims = var->dims;
	List dimsizes = var->dimsizes;
	List indices = vc->indices;
	int size = 1;
	int *idxArray = safeMalloc(sizeof(int));
	
	idxArray[0] = var->globalIndex;
	while(dimsizes != NULL) {
		int dimsize = calcNumExp(dimsizes->item, NULL);
		int elemsmult = calcElemsSize(dimsizes->next, substset);
		int *idxArray2 = safeMalloc(size*dimsize*sizeof(int));
		int saveIdx = 0;
		if(indices != NULL) {
			List subdoms = indices->item;
			while(subdoms != NULL) {
				Subdomain subdom = subdoms->item;
				int min = calcNumExp(subdom->min, substset);
				int max = calcNumExp(subdom->max, substset);
				if(min < 0 || max >= dimsize) {
					char str[strlen(varname)+100];
					strcpy(str, "Index out of bounds (variable '");
					strcat(str, varname);
					strcat(str, "')\n");
					error(str);
					exit(-1);
				}
				for(int i = min; i <= max; i++) {
					for(int j = 0; j < size; j++) {
						idxArray2[saveIdx] = idxArray[j] + i*elemsmult;
						saveIdx++;
					}
				}
				subdoms = subdoms->next;
			}	
			indices = indices->next;
		} else {
			for(int i = 0; i < dimsize; i++) {
				for(int j = 0; j < size; j++) {
					idxArray2[saveIdx] = idxArray[j] + i*elemsmult;
					saveIdx++;
				}
			}
		}
		size = saveIdx;
		dims = dims-1;
		dimsizes = dimsizes->next;
		
		int *backup = idxArray;
		idxArray = idxArray2;
		free(backup);
	}
	return (sizedArray) {size, idxArray};
}

void rewriteVarCall(VarCall vc, List substset, int domainset) {
	sizedArray indices = obtainVarCallIndices(vc, substset);
	
	printNFVar(indices.array[0]);
	for(int i = 1; i < indices.size; i++) {
		printComma(1);
		printNFVar(indices.array[i]);
	}
	if(domainset) {
		char *varname = vc->name;
		Variable var = getVariableFromDB(varname);
		for(int i = 0; i < indices.size; i++) {
			domainVarSet(var, indices.array[i]);
		}
	}
	free(indices.array);
}

void rewriteVarSubstCall(VarCall vc, List substset) {
	char *name = vc->name;
	Variable var = getVariableFromDB(name);
	if(var == NULL) {
		fprintf(stdout, "%d", getSubstitution(name, substset)->val);
	} else {
		sizedArray indices = obtainVarCallIndices(vc, substset);
		
		if(indices.size == 0) {
			error("These indices lead to an empty set of values.");
		}
		if(indices.size > 1) {
			error("Type conflict: only a single value is allowed here.");
		}
		
		printNFVar(indices.array[0]);
		for(int i = 1; i < indices.size; i++) {
			printComma(1);
			printNFVar(indices.array[i]);
		}
		free(indices.array);
	}
}

void rewriteVarList(List vl, List substset, int domainset) {
	rewriteVarCall(vl->item, substset, domainset);
	vl = vl->next;
	while(vl != NULL) {
		printComma(1);
		rewriteVarCall(vl->item, substset, domainset);
		vl = vl->next;
	}
}

void rewriteSubDomain(Subdomain d, List substset) {
	fprintf(stdout, "%d..%d", calcNumExp(d->min, substset), calcNumExp(d->max, substset));
}

void rewriteDomain(List subdoms, List substset) {
	printBracketOpen();
	rewriteSubDomain(subdoms->item, substset);
	subdoms = subdoms->next;
	while(subdoms != NULL) {
		printComma(1);
		rewriteSubDomain(subdoms->item, substset);
		subdoms = subdoms->next;
	}
	printBracketClose();
}

void rewriteDomainSpec(RecursiveType domspec, List substset) {
	if(domspec->type == DOMSET) {
		DomainSet domset = (DomainSet) domspec->data;
		rewriteVarList(domset->varlist, substset, 1);
		fprintf(stdout, " <- ");
		rewriteDomain(domset->doms, substset);
		printLineEnd();
	} else { /* domainspec->type == FORALL */
		ForAll forall = (ForAll) domspec->data;
		char *varname = forall->varname;
		List subdomains = forall->subdomains;
		while(subdomains != NULL) {
			Subdomain subdom = subdomains->item;
			int i;
			for(i = calcNumExp(subdom->min, substset); i <= calcNumExp(subdom->max, substset); i++) {
				Substitution subst = newSubstitution(varname, i);
				List newset = newList(subst, substset);
				List domspeclist = forall->items;
				while(domspeclist != NULL) {
					rewriteDomainSpec(domspeclist->item, newset);
					domspeclist = domspeclist->next;
				}
				free(newset);
				
			}
			subdomains = subdomains->next;
		}
	}
}

sizedArray mergedSets(List varcalls, List substset) {
	sizedArray indices;
	indices.size = 0;
	indices.array = safeMalloc(0*sizeof(int));
	while(varcalls != NULL) {	
		VarCall vc = varcalls->item;
		Variable var = getVariableFromDB(vc->name);
		if(var == NULL) {
			char errorstr[1000];
			strcpy(errorstr, vc->name);
			strcat(errorstr, " is not defined here.");
			error(errorstr);
		}
	
		sizedArray varindices = obtainVarCallIndices(vc, substset);
	
		if(varindices.size != 0) {		
			indices.array = realloc(indices.array, (indices.size+varindices.size)*sizeof(int));
			assert(indices.array != NULL);
			memcpy(&(indices.array[indices.size]), varindices.array, varindices.size*sizeof(int));
			indices.size += varindices.size;
		}
		free(varindices.array);
		varcalls = varcalls->next;
	}
	return indices;
}

void rewriteAllDiff(List varcalls, List substset) {
	sizedArray indices = mergedSets(varcalls, substset);
	
	for(int i = 0; i < indices.size-1; i++) {
		for(int j = i+1; j < indices.size; j++) {
			printNFVar(indices.array[i]);
			fprintf(stdout, " <> ");
			printNFVar(indices.array[j]);
			if(i != indices.size-2 || j != indices.size-1) {
				printLineEnd();
			}
		}
	}
	free(indices.array);
}

void rewriteRelation(List varcalls, List substset, char symbol) {
	sizedArray indices = mergedSets(varcalls, substset);
	
	for(int i = 0; i < indices.size-1; i++) {
		printNFVar(indices.array[i]);
		fprintf(stdout, " %c ", symbol);
		printNFVar(indices.array[i+1]);
		if(i+1 != indices.size -1) {
			fprintf(stdout, "; ");
		}

	}
	fprintf(stdout, "\n");
	free(indices.array);
}

void rewriteEqual(List varcalls, List substset) {
	rewriteRelation(varcalls, substset, '=');
}

void rewriteIncreasing(List varcalls, List substset) {
	rewriteRelation(varcalls, substset, '<');
}

void rewriteDecreasing(List varcalls, List substset) {
	rewriteRelation(varcalls, substset, '>');
}

void rewriteSum(List varcalls, List substset) {
	sizedArray indices = mergedSets(varcalls, substset);
	
	if(indices.size == 0) {
		fprintf(stdout, "(0)");
		free(indices.array);
		return;
	}
	fprintf(stdout, "(");
	printNFVar(indices.array[0]);
	for(int i = 1; i < indices.size; i++) {
		fprintf(stdout, " + ");
		printNFVar(indices.array[i]);
	}
	fprintf(stdout, ")");
	free(indices.array);
}

void rewriteProduct(List varcalls, List substset) {
	sizedArray indices = mergedSets(varcalls, substset);
	
	if(indices.size == 0) {
		fprintf(stdout, "(1)");
		free(indices.array);
		return;
	}
	fprintf(stdout, "(");
	printNFVar(indices.array[0]);
	for(int i = 1; i < indices.size; i++) {
		fprintf(stdout, " * ");
		printNFVar(indices.array[i]);
	}
	fprintf(stdout, ")");
	free(indices.array);
}

void rewriteMaximum(List varcalls, List substset) {
	sizedArray indices = mergedSets(varcalls, substset);
	
	if(indices.size == 0) {
		error("@maximum: maximum over an empty set.");
	}
	for(int i = 0; i < indices.size-1; i++) {
		fprintf(stdout, "max(");
		printNFVar(indices.array[i]);
		fprintf(stdout, ", ");
	}
	printNFVar(indices.array[indices.size-1]);
	for(int i = 0; i < indices.size-1; i++) {
		fprintf(stdout, ")");
	}
	free(indices.array);
}

void rewriteMinimum(List varcalls, List substset) {
	sizedArray indices = mergedSets(varcalls, substset);
	
	if(indices.size == 0) {
		error("@minimum: minimum over an empty set.");
	}
	for(int i = 0; i < indices.size-1; i++) {
		fprintf(stdout, "min(");
		printNFVar(indices.array[i]);
		fprintf(stdout, ", ");
	}
	printNFVar(indices.array[indices.size-1]);
	for(int i = 0; i < indices.size-1; i++) {
		fprintf(stdout, ")");
	}
	free(indices.array);
}

void rewriteFunctionCall(FunctionCall fc, List substset) {
	List args = fc->argList;
	switch(fc->type) {
		case MAXMIN:
			fprintf(stdout, "%s", fc->funcName);
			fprintf(stdout, "(");
			rewriteNumExp(args->item, substset);
			fprintf(stdout, ",");
			rewriteNumExp(args->next->item, substset);
			fprintf(stdout, ")");
			break;
		case ALLANY:
			fprintf(stdout, "%s", fc->funcName);
			fprintf(stdout, "(");
			rewriteConstraint(args->item, substset);
			args = args->next;
			while(args != NULL) {
				fprintf(stdout, ", ");
				rewriteConstraint(args->item, substset);
				args = args->next;
			}
			fprintf(stdout, ")");
			break;
		case ABS:
			fprintf(stdout, "%s", fc->funcName);
			fprintf(stdout, "(");
			rewriteNumExp(args->item, substset);
			fprintf(stdout, ")");
			break;
		case ALLDIFF:
			rewriteAllDiff(args, substset);
			break;
		case EQUAL:
			rewriteEqual(args, substset);
			break;
		case INCREASING:
			rewriteIncreasing(args, substset);
			break;
		case DECREASING:
			rewriteDecreasing(args, substset);
			break;
		case SUM:
			rewriteSum(args, substset);
			break;
		case PRODUCT:
			rewriteProduct(args, substset);
			break;
		case MAXIMUM:
			rewriteMaximum(args, substset);
			break;
		case MINIMUM:
			rewriteMinimum(args, substset);
			break;
		default:
			error("FunctionCall has wrong type.\n");
	}
}

void rewriteValue(Value v, List substset) {
	debug("rewrite value\n");
	switch(v->type) {
		case INT:
			fprintf(stdout, "%d", atoi(v->data));
			break;
		case VARSUBSTCALL:
			debug("varsubstcall\n");
			rewriteVarSubstCall(v->data, substset);
			break;
		case FUNCTIONCALL:
			rewriteFunctionCall(v->data, substset);
			break;
		case NUMEXP:
			fprintf(stdout, "(");
			rewriteNumExp(v->data, substset);
			fprintf(stdout, ")");
			break;
		default:
			error("Value has wrong type.\n");
	}
}

void rewriteFactor(Factor f, List substset) {
	debug("\nrewrite Factor\n");
	if(f->type == VALUE) {
		rewriteValue(f->data, substset);
	} else { /* f->type == NEGATION */
		fprintf(stdout, "-");
		rewriteFactor(f->data, substset);
	}
}

void rewriteTerm(Term t, List substset) {
	debug("\nrewrite term\n");
	List factorList = t->factorList;
	List operatorList = t->operatorList;
	rewriteFactor(factorList->item, substset);
	factorList = factorList->next;
	while(factorList != NULL) {
		fprintf(stdout, " %s ", (char *) operatorList->item);
		rewriteFactor(factorList->item, substset);
		factorList = factorList->next;
		operatorList = operatorList->next;
	}
}

void rewriteNumExp(NumExp exp, List substset) {
	List termList = exp->termList;
	List operatorList = exp->operatorList;
	rewriteTerm(termList->item, substset);
	termList = termList->next;
	while(termList != NULL) {
		fprintf(stdout, " %s ", (char *) operatorList->item);
		rewriteTerm(termList->item, substset);
		termList = termList->next;
		operatorList = operatorList->next;
	}
}

void rewriteConstraint(Constraint c, List substset) {
	rewriteNumExp(c->exp1, substset);
	if(c->exp2 != NULL) {
		fprintf(stdout, " %s ", c->operator);
		rewriteNumExp(c->exp2, substset);	
	}
}

void rewriteConstraintSpec(RecursiveType cs, List substset) {
	if(cs->type == CONSTRAINT) {
		Constraint c = (Constraint) cs->data;
		rewriteConstraint(c, substset);
		printLineEnd();
	} else { /* cs->type == FORALL */
		ForAll forall = cs->data;
		char *varname = forall->varname;
		List subdomains = forall->subdomains;
		while(subdomains != NULL) {
			Subdomain subdom = subdomains->item;
			for(int val = calcNumExp(subdom->min, substset); val <= calcNumExp(subdom->max, substset); val++) {
				Substitution subst = newSubstitution(varname, val);
				List newset = newList(subst, substset);
				List cslist = forall->items;
				while(cslist != NULL) {
					rewriteConstraintSpec(cslist->item, newset);
					cslist = cslist->next;
				}
			}
			subdomains = subdomains->next;
		}
	}
}
