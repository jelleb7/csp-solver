#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "datatypes.h"
#include "variable.h"
#include "constraint.h"
#include "solve.h"
#include <limits.h>

extern FILE *logFile;

Constraint newValConstraint(NumExp e1) {
	Constraint c = safeMalloc(sizeof(constraint));
	c->arity = 0;
	c->exp1 = e1;
	c->exp2 = NULL;
	c->vars = emptyIntegerSet();
	return c;
}

Constraint newBoolConstraint(NumExp e1, RelOperator relop, NumExp e2) {
	Constraint c = newValConstraint(e1);
	c->op = relop;
	c->exp2 = e2;
	return c;
}

void setOperatorOfConstraint(Constraint c, RelOperator relop) {
	c->op = relop;
}

void setSecondExp(Constraint c, NumExp exp2) {
	c->exp2 = exp2;
}

void setIndexOfConstraint(Constraint c, int index) {
	c->index = index;
}	

void addVarToConstraint(Constraint c, Variable v) {
	addIntegerToSet(c->vars, indexOfVar(v));
}

void removeVarFromConstraint(Constraint c, Variable v) {
  removeIntegerFromSet(c->vars, indexOfVar(v));
}

NumExp firstExp(Constraint c) {
	return c->exp1;
}

NumExp secondExp(Constraint c) {
	return c->exp2;
}

RelOperator operatorOfConstraint(Constraint c) {
	return c->op;
}

int indexOfConstraint(Constraint c) {
	return c->index;
}

void freeConstraint(Constraint c) {
	freeNumExp(c->exp1);
	if(c->exp2 != NULL) {
		freeNumExp(c->exp2);
	}
	freeIntegerSet(c->vars);
	free(c);
}

ConstraintList addConstraint(ConstraintList list, Constraint constraint) {
	ConstraintList temp = list;
	ConstraintList new = safeMalloc(sizeof(constraintList));
	new->constraint = constraint;
	new->next = NULL;
	if(list == NULL) {
		return new;
	}
	while(temp->next != NULL) {
		temp = temp->next;
	}
	if(temp->next == NULL) {
		temp->next = new;
	}
	return list;
}

void freeConstraintList(ConstraintList cl) {
	if(cl != NULL) {
		freeConstraintList(cl->next);
		free(cl);
	}
}

NumExp newNumExp(void *data) {
	NumExp numExp = safeMalloc(sizeof(numExp));
	numExp->data = data;
	numExp->next = NULL;
	return numExp;
}

void freeNumExp(NumExp numexp) {
	if(numexp->next == NULL) {
		freeTerm(numexp->data);
	} else {
		freeNumExp(numexp->data);
		freeNumExp(numexp->next);
	}
	free(numexp);
}

int matchVarExpression(Variable v, NumExp exp) {
	Term t;
	Factor f;
	if(exp->next != NULL) {
		return 0;
	}
	t = exp->data;
	if(t->next != NULL) {
		return 0;
	}
	f = t->data;
	if(f->type == NUMNEG) {
		return 0;
	}
	Value val = f->data.value;
	return (val->type == VARVAL && val->data.varIndex == v->index);
}

NumExp addTerm(NumExp old, TermOperator termop, Term t) {
	NumExp new = newNumExp(old);
	new->termop = termop;
	new->next = newNumExp(t);
	return new;
}

int singletonNumExp(NumExp n, Problem p) {
	if(n == NULL) {
		return 0;
	}
	if(n->next == NULL) {
		return singletonTerm(n->data, p);
	}
	return singletonNumExp(n->data, p)*singletonNumExp(n->next, p);
}

Term newTerm(void *data) {
	Term t = safeMalloc(sizeof(term));
	t->next = NULL;
	t->data = data;
	return t;
} 

void freeTerm(Term t) {
	if(t->next == NULL) {
		freeFactor(t->data);
	} else {
		freeTerm(t->next);
		freeTerm(t->data);
	}
	free(t);
}

Term addFactor(Term old, FactorOperator factorop, Factor f) {
	Term new = newTerm(old);
	new->factorop = factorop;
	new->next = newTerm(f);
	return new;
}

int singletonTerm(Term t, Problem p) {
	if(t->next == NULL) {
		return singletonFactor(t->data, p);
	}
	return singletonTerm(t->data, p)*singletonTerm(t->next, p);
}

Factor newFactor(FactorType type) {
	Factor f = safeMalloc(sizeof(factor));
	f->type = type;
	return f;
}

void freeFactor(Factor f) {
	if(f->type == NUMNEG) {
		freeFactor(f->data.factor);
	} else { /* VALUE */
		freeValue(f->data.value);
	}
	free(f);
}

FactorType typeOfFactor(Factor f) {
	return f->type;
}

Factor subFactorOfFactor(Factor f) {
	return f->data.factor;
}

Value subValueOfFactor(Factor f) {
	return f->data.value;
}

FunctionCall newFunctionCall(FunctionName name, int argc, void *argv) {
	FunctionCall f = safeMalloc(sizeof(functionCall));
	f->name = name;
	f->argc = argc;
	f->argv = argv;
	return f;
}

void freeFunctionCall(FunctionCall f) {
	int i;
	ConstraintList *lists;
	NumExp *exps;
	if(f->name == ANY || f->name == ALL) {
		lists = (ConstraintList *) f->argv;
		for(i = 0; i < f->argc; i++) {
			freeConstraintList(lists[0]);
		}
	} 
	else {
		exps = (NumExp *) f->argv;
		for(i = 0; i < f->argc; i++) {
			freeNumExp(exps[i]);
		}
	}
	free(f->argv);
	free(f);
}

FunctionName nameOfFunctionCall(FunctionCall fc) {
	return fc->name;
}

void *argsOfFunctionCall(FunctionCall fc) {
	return fc->argv;
}

int argCountOfFunctionCall(FunctionCall fc) {
	return fc->argc;
}

Factor newValueFactor(Value v) {
	Factor f = newFactor(VALUE);
	f->data.value = v;
	return f;
}

Factor newMinusFactor(Factor next) {
	Factor f = newFactor(NUMNEG);
	f->data.factor = next;
	return f;
}

int singletonFactor(Factor f, Problem p) {
	if(f->type == NUMNEG) {
		return singletonFactor(f->data.factor, p);
	}
	return singletonValue(f->data.value, p);
}


Value newValue(ValueType type) {
	Value v = safeMalloc(sizeof(value));
	v->type = type;
	v->exponent = NULL;
	return v;
}

void freeValue(Value v) {
	if(v->type == NUMEXP) {
		freeNumExp(v->data.numexp);
	} else if(v->type == FUNCVAL) {
		freeFunctionCall(v->data.funcCall);
	}
	if(v->exponent != NULL) {
		freeFactor(v->exponent);
	}
	free(v);
}

int singletonValue(Value v, Problem p) {
	if(v->type == VARVAL) {
		if(!singletonDomain(domainOfVar(varByIndex(p, v->data.varIndex)))) {
			return 0;
		}
	} else if(v->type == NUMEXP) {
		if(!singletonNumExp(v->data.numexp, p)) {
			return 0;
		}
	} else if(v->type == FUNCVAL) { 
		int i;
		if(v->data.funcCall->name == ANY || v->data.funcCall->name == ALL) {
			ConstraintList *lists = v->data.funcCall->argv;
			ConstraintList list = lists[0];
			while(list != NULL) {
				if(!singletonNumExp(firstExp(list->constraint), p)) {
					return 0;
				}	
				NumExp exp2  = secondExp(list->constraint);
				if(exp2 != NULL && !singletonNumExp(exp2, p)) {
					return 0;
				}
				list = list->next;
			}
		} else {
			NumExp *numExps = v->data.funcCall->argv;
			for(i = 0; i < v->data.funcCall->argc; i++) {
				if(!singletonNumExp(numExps[i], p)) {
					return 0;
				}
			}
		}
	}
	return (v->exponent == NULL || singletonFactor(v->exponent, p));
}

Value newIntVal(int intval) {
	Value v = newValue(INTVAL);
	v->data.intval = intval;
	return v;
}

Value newVarVal(int varIndex) {
	Value v = newValue(VARVAL);
	v->data.varIndex = varIndex;
	return v;
}

Value newFuncVal(FunctionCall fc) {
	Value v = newValue(FUNCVAL);
	v->data.funcCall = fc;
	return v;
}

Value newNumExpVal(NumExp numexp) {
	Value v = newValue(NUMEXP);
	v->data.numexp = numexp;
	return v;
}

void printRelOperator(RelOperator op) {
	char *relops[] = {"=", "<", ">", ">=", "<=", "<>"};
	addLog(" %s ", relops[op]);
}

void printTermOperator(TermOperator op) {
	char *termops[] = {"-", "+"};
	addLog(" %s ", termops[op]);
}

void printFactorOperator(FactorOperator op) {
	char *factorops[] = {"*", "/", "%"};
	addLog(" %s ", factorops[op]);
}

void printValue(Value v) {
	char *functions[] = {"MAX", "MIN", "ABS", "ANY", "ALL"};
	int i;
	switch(v->type) {
		case NUMEXP:
			addLog("(");
			printNumExp(v->data.numexp);
			addLog(")");
			break;
		case INTVAL:
			addLog("%d", v->data.intval);
			break;
		case FUNCVAL:
			addLog("%s(", functions[v->data.funcCall->name]);
			if(v->data.funcCall->name == ANY || v->data.funcCall->name == ALL) {
				ConstraintList *lists = v->data.funcCall->argv;
				ConstraintList list = lists[0];
				while(list != NULL) {
					printNumExp(firstExp(list->constraint));
					if(secondExp(list->constraint) != NULL) {
						printRelOperator(operatorOfConstraint(list->constraint));
						printNumExp(secondExp(list->constraint));
					}
					list = list->next;
				}
			} else {
				NumExp *numExps = v->data.funcCall->argv;
				printNumExp(numExps[0]);
				for(i = 1; i < v->data.funcCall->argc; i++) {
					addLog(", ");
					printNumExp(numExps[i]);	
				}
			}
			addLog(")");
			break;
		default: /* VARVAL */
			addLog("X%d", v->data.varIndex);
			break;
	}
	if(v->exponent != NULL) {
		addLog("^");
		printFactor(v->exponent);
	}
}

void printFactor(Factor f) {
	if(f->type == NUMNEG) {
		addLog("-(");
		printFactor(f->data.factor);
		addLog(")");
	} else {	/* VALUE */
		printValue(f->data.value);
	}
}

void printTerm(Term t) {
	if(t->next == NULL) {
		printFactor(t->data);
	} else {
		addLog("(");
		printTerm(t->data);
		addLog(")");
		printFactorOperator(t->factorop);
		addLog("(");
		printTerm(t->next);
		addLog(")");
	}
}

void printNumExp(NumExp numExp) {
	if(numExp->next == NULL) {
		printTerm(numExp->data);
	} else {
		addLog("(");
		printNumExp(numExp->data);
		addLog(")");
		printTermOperator(numExp->termop);
		addLog("(");
		printNumExp(numExp->next);
		addLog(")");
	}
}

void printConstraint(Constraint c) {
	printNumExp(c->exp1);
	if(c->exp2 != NULL) {
		printRelOperator(c->op);
		printNumExp(c->exp2);
	}
	addLog("\n");
}

void printConstraintList(ConstraintList cl) {
	while(cl !=  NULL) {
		addLog("\t\t");
		printConstraint(cl->constraint);
		cl = cl->next;
	}
}

int *varIndicesOfConstraint(Constraint c) {
	return valuesOfSet(c->vars);
}

/* returns the maximum of numerical expressions exp1 and exp2 */
int max(NumExp exp1, NumExp exp2, Problem p) {
	int a, b;
	a = calcExp(exp1, p);
	b = calcExp(exp2, p);
	return (a > b ? a : b);
}

/* returns the minimum of numerical expressions exp1 and exp2 */
int min(NumExp exp1, NumExp exp2, Problem p) {
	int a, b;
	a = calcExp(exp1, p);
	b = calcExp(exp2, p);
	return (a < b ? a : b);
}

/* returns the absolute value of numerical expression exp1 */
int absVal(NumExp exp1, Problem p) {
	int val = calcExp(exp1, p);
	return (val >= 0 ? val : -val);
}

/* calculates the value that is returned by FunctionCall fc */
int calcFuncVal(FunctionCall fc, Problem p) {
	FunctionName funcName = nameOfFunctionCall(fc);
	void *arguments = argsOfFunctionCall(fc);
	int argCount = argCountOfFunctionCall(fc);
	if(funcName == ANY || funcName == ALL) {	/* nr of arguments is 1 */
		ConstraintList *lists = arguments;
		if(funcName == ANY) {
			return or(lists[0], p);
		}
		return and(lists[0], p);
	}
	/* not ANY or ALL -> MAX, MIN or ABS */
	NumExp *numExps = arguments;
	if(argCount == 2) { /* MAX or MIN */
		if(funcName == MIN) {
			return min(numExps[0], numExps[1], p);
		}
		return max(numExps[0], numExps[1], p);
	}
	/* argCount == 1 -> ABS */
	return absVal(numExps[0], p);
}

/* calculates the value of Value v */
int calcValue(Value v, Problem p) {
	int base;
	if(v->type == VARVAL) {
		base = domainMinimumOfVar(varByIndex(p, v->data.varIndex));
	} else if(v->type == NUMEXP) {
		base = calcExp(v->data.numexp, p);
	} else if(v->type == FUNCVAL) {
		base = calcFuncVal(v->data.funcCall, p);
	} else {
		base = v->data.intval;
	}
	if(v->exponent != NULL) {
		return pow(base, calcFactor(v->exponent, p));
	}
	return base;
}

/* calculates the value of Factor f */
int calcFactor(Factor f, Problem p) {
	if(typeOfFactor(f) == NUMNEG) {
		return -calcFactor(subFactorOfFactor(f), p);
	}
	return calcValue(subValueOfFactor(f), p);
}

/* calculates the value of Term t */
int calcTerm(Term t, Problem p) {
	if(t->next == NULL) {
		return calcFactor(t->data, p);
	}
	int val1 = calcTerm(t->data, p);
	int val2 = calcTerm(t->next, p);
	if(t->factorop == MUL) {
		return (val1 * val2);
	}
	/* div or mod */
	if(val2 == 0) {
		addLog("Division by zero: abort\n");
		exit(-1);	
	}
	if(t->factorop == DIV) {
		return (val1 / val2);
	}
	return (val1 % val2);
}

/* calculates the value of NumExp exp */
int calcExp(NumExp exp, Problem p) {
	if(exp->next != NULL) {
		int val1 = calcExp(exp->data, p);
		int val2 = calcExp(exp->next, p);
		if(exp->termop == PLUS) {
			return (val1 + val2);
		}
		return (val1 - val2);
	}
	return calcTerm(exp->data, p);
}

int arityOfConstraint(Constraint c) {
  return sizeOfSet(c->vars);
}

/* checks if all variables are assigned a value */
int determinable(Constraint c, Problem p) {
	return (arityOfConstraint(c) == 0);
}


/*
Constraint replaceVarInConstraint(Constraint c, Variable v);
NumExp replaceVarInExp(NumExp numexp, Variable v);
Factor replaceVarInFactor(Factor f, Variable v);

ConstraintList replaceVarInConstraintList(ConstraintList cl, Variable var) {
  if(cl == NULL) {
    return NULL;
  }
  ConstraintList new = safeMalloc(sizeof(ConstraintList));
  new->constraint = replaceVarInConstraint(cl->constraint, var);
  new->next = replaceVarInConstraintList(cl->next, var);
  return new;
}

FunctionCall replaceVarInFuncCall(FunctionCall func, Variable var) {
  FunctionCall new = safeMalloc(sizeof(functionCall));
	new->name = func->name;
	new->argc = func->argc;
	
	if(func->name == ANY || func->name == ALL) {
	  new->argv = replaceVarInConstraintList(func->argv, var);
	} else { 
	  NumExp *oldArgs = func->argv;
	  NumExp *newArgs = safeMalloc(new->argc * sizeof(NumExp));
	  int i;
	  for(i = 0; i < new->argc; i++) {
	    newArgs[i] = replaceVarInExp(oldArgs[i], var);
	  }
	  new->argv = newArgs;
	}
  
  return new;
}

Value replaceVarInValue(Value val, Variable var) {
  Value new = newValue(val.type);
  
  if(val.type == VARVAL && val.data.varIndex == getVariableIndex(var)) {
    new.type = INTVAL;
    new.data.intval = getDomainMinimum(getVarDomain(var));
  } else if(val.type == NUMEXP) {
    new.data.numexp = replaceVarInExp(val.data.numexp, var);
  } else if(val.type == FUNCVAL) {
    new.data.funcCall = replaceVarInFuncCall(val.data.funcCall, var);
  } else {
    new.data.intval = val.data.intval;
  }
  
	if(val.exponent != NULL) {
	  new.exponent = replaceVarInFactor(val.exponent, var);
	}
	
	return new;
}

Factor replaceVarInFactor(Factor f, Variable v) {
  Factor new = safeMalloc(sizeof(factor));
	new->type = f->type;
	
	if(new->type == NUMNEG) {
	  new->data.factor = replaceVarInFactor(f->data.factor, v);
	} else {
	  new->data.value = replaceVarInValue(f->data.value, v);
	}
	
	return new;
}

Term replaceVarInTerm(Term t, Variable v) {
  Term new = safeMalloc(sizeof(term));
  if(t->next == NULL) {
    new->next = NULL;
    new->data = replaceVarInFactor(t->data, v);
  } else {
    new->data = replaceVarInTerm(t->data, v);
    new->next = replaceVarInTerm(t->next, v);
    new->factorop = t->factorop;
  }
	return new;
}

NumExp replaceVarInExp(NumExp numexp, Variable v) {
	NumExp new = safeMalloc(sizeof(numExp));	
	if(numexp->next != NULL) {
		new->data = replaceVarInExp(numexp->data, v);
		new->next = replaceVarInExp(numexp->next, v);
	} else {
	  new->next = NULL;
	  new->data = replaceVarInTerm(numexp->data, v);
	  new->termop = numexp->termop;
	}
	return new;
}

Constraint replaceVarInConstraint(Constraint c, Variable v) {
	Constraint new = safeMalloc(sizeof(constraint));
	new->arity = c->arity-1;
	new->exp1 = replaceVarInExp(c->exp1, v);
	new->exp2 = replaceVarInExp(c->exp2, v);
	new->op = c->op;
	new->vars = copyIntegerSet(c->vars);
	removeIntegerFromSet(new->vars, v->index);
	return new;
}
*/

/* checks if the constraint c is satisfiable */
int satisfiable(Constraint c, Problem p) {
	int *varIndices = varIndicesOfConstraint(c);
	IntegerSet fullDomain;
	Variable unassignedVar = NULL;
	int *values;
	int i, j;
	int satisfied = 0;
	int arity = arityOfConstraint(c);
	
	/* search for variable that is not yet assigned */	
	for(i = 0; i < arity; i++) {
	  unassignedVar = varByIndex(p, varIndices[i]);
	  fullDomain = domainOfVar(unassignedVar);
	  if(sizeOfSet(fullDomain) != 1) {
	    break;
	  }
	}
	
	/* all variables have a singleton domain -> one possibility */
	if(unassignedVar == NULL || i == arity) { 
		return checkConstraint(c, p);
	} 
	
	/* a variable that is not assigned yet is found */
	values = valuesOfSet(fullDomain);
	for(j = 0; j < sizeOfSet(fullDomain); j++) {
		setDomainOfVar(unassignedVar, createSingletonDomain(values[j]));
		satisfied = satisfiable(c, p);
		freeIntegerSet(domainOfVar(unassignedVar));
		if(satisfied) {
			break;
		}
	}
	
	/* reset old domain of the ith variable */
	setDomainOfVar(unassignedVar, fullDomain);		
	return satisfied;
}



/* checks if there is at least one constraint true in list */
int or(ConstraintList list, Problem p) {
	while(list != NULL) {
		if(checkConstraint(list->constraint, p)) {
			return 1;
		}
		list = list->next;
	}
	return 0;
}

/* checks if all constraints are true in list */
int and(ConstraintList list, Problem p) {
	while(list != NULL) {
		if(!checkConstraint(list->constraint, p)) {
			return 0;
		}
		list = list->next;
	}
	return 1;
}


/* switches the comparison operator if possible */
RelOperator switchOperator(RelOperator relop) {
	if(relop == GEQ) {
		return LEQ;
	}
	if(relop == LEQ) {
		return GEQ;
	}
	if(relop == GREATER) {
		return SMALLER;
	}
	if(relop == SMALLER) {
		return GREATER;
	}
	return relop;
}

/* 
  checks if the comparison operator relop between left and right is applicable 
*/
int checkRelation(int left, RelOperator relop, int right) {
	if(relop == NEQ) {
		return (left != right);
	}
	if(relop == SMALLER) {
		return (left < right);
	}
	if(relop == GREATER) {
		return (left > right);
	}
	if(relop == GEQ) {
		return (left >= right);
	}
	if(relop == LEQ) {
		return (left <= right);
	}
	return (left == right); /* relop == IS */
}

int checkConstraint(Constraint c, Problem p) {
	int val1 = calcExp(firstExp(c), p);
	if(secondExp(c) != NULL) {
		int val2 = calcExp(secondExp(c), p);
		return checkRelation(val1, operatorOfConstraint(c), val2);
	} 
	return val1;
}
