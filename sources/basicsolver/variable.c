#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "datatypes.h"
#include "problem.h"
#include "variable.h"
#include "solve.h"

extern FILE *logFile;
extern int (*varOrdered)(Variable, Variable);
  
/* BEGIN functions Variable */
Variable newVariable(int varIndex, DataType d) {
	Variable v = safeMalloc(sizeof(variable));
	v->index = varIndex;
	v->constraintDegree = 0;
	v->connectivity = 0;
	v->varConnections = 0;
	v->type = d;
	v->domain = emptyIntegerSet();
	v->constraints = emptyIntegerSet();
	v->sequencePos = NULL;	
	return v;
}

void setDomainOfVar(Variable v, IntegerSet domain) {
	v->domain = domain;
}

void freeVariable(Variable v) {
  if(v != NULL) {
	  freeIntegerSet(v->domain);
	  freeIntegerSet(v->constraints);
	  free(v);
	}
}

void addConstraintToVar(Variable v, Constraint c) {
	addIntegerToSet(v->constraints, indexOfConstraint(c));
}

void removeConstraintFromVar(Variable v, Constraint c) {
	removeIntegerFromSet(v->constraints, indexOfConstraint(c));
}

int indexOfVar(Variable v) {
	return v->index;
}

DataType dataTypeOfVar(Variable v) {
  return v->type;
}

int constraintAmountOfVar(Variable v) {
	return v->constraints->size;
}

int *constraintIndicesOfVar(Variable v) {
	return v->constraints->values;
}

int isAssigned(Variable v, Problem p) {
	return !varInSequence(v, varSeqOfProblem(p));
}

IntegerSet domainOfVar(Variable v) {
	return v->domain;
}

VarPos sequencePosition(Variable v) {
	return v->sequencePos;
}

void setSeqPosOfVar(Variable v, VarPos pos) {
	v->sequencePos = pos;
}

void lowerDegree(Variable v) {
	v->constraintDegree--;
}

void plusDegree(Variable v) {
	v->constraintDegree++;
}

int degreeOfVar(Variable v) {
  return v->constraintDegree;
}

void lowerConnectivity(Variable v) {
  v->connectivity--;
}

void plusConnectivity(Variable v) {
  v->connectivity++;
}

/* END functions Variable */

/* BEGIN functions Domain */

IntegerSet createSingletonDomain(int value) {
	IntegerSet singleton = emptyIntegerSet();
	addIntegerToSet(singleton, value);
	return singleton;
}

void printDomainOfVar(Variable v) {
  IntegerSet domain = v->domain;
	int i;
	int *values = valuesOfSet(domain);
	addLog("[");
	if(sizeOfSet(domain) > 0) {
		addLog("%d", values[0]);
	}
	for(i = 1; i < sizeOfSet(domain); i++) {
		addLog(", %d", values[i]);
	}
	addLog("]");
}

int singletonDomain(IntegerSet domain) {
	return (sizeOfSet(domain) == 1);
}

int domainSizeOfVar(Variable v) {
	return sizeOfSet(v->domain);
}

int *domainValuesOfVar(Variable v) {
	return valuesOfSet(v->domain);
}

int domainMinimumOfVar(Variable v) {
	return minimumOfSet(v->domain);
}

int domainMaximumOfVar(Variable v) {
	return maximumOfSet(v->domain);
}

/* END functions Domain */

/*
 * creates a new VarList and sets the first item
*/ 
VarList newVarList(int varIndex, VarList next) {
	VarList vl = safeMalloc(sizeof(varList));
	vl->varIndex = varIndex;
	vl->next = next;
	return vl;
}

/*
 * 
*/ 
void freeVarList(VarList vl) {
	if(vl != NULL) {
		freeVarList(vl->next);
		free(vl);
	}
}

/*
 * creates a Variable, given  to a VarList
*/ 
void addVar(VarList vl, int varIndex) {
	VarList end = vl;
	while(end->next != NULL) {
		end = end->next;
	}
	end->next = newVarList(varIndex, NULL);
}

/*
 * 
*/ 
VarList addVars(VarList vl, DataType d, VarList toAdd) {
	VarList tempList = toAdd;
	while(tempList != NULL) {
		tempList->varType = d;
		tempList = tempList->next;
	}
	if(vl == NULL) {
		return toAdd;
	}
	tempList = vl;
	while(tempList->next != NULL) {
		tempList = tempList->next;
	}
	tempList->next = toAdd;
	return vl;
}


/* END functions VarSet */


/* BEGIN functions Variable */

/*
 * 
*/ 
void printVar(Variable v) {
	addLog("X%d : %s\n", v->index, (v->type == INTEGER ? "integer" : "boolean"));
	addLog("\tDomain(%d): ", sizeOfSet(v->domain));
	printDomainOfVar(v);
	addLog("\n");
	addLog("\tDegree(%d): \n", v->constraintDegree);
	addLog("\tConnectivity(%d): \n", v->connectivity);
	addLog("\n");
}


/* END functions Variable */


/* BEGIN Functions VarSeq */

/*
 * 
*/ 
VarSeq emptyVarSeq() {
	VarSeq seq = safeMalloc(sizeof(varSeq));
	seq->first = NULL;
	return seq;
}

/*
 * 
*/ 
VarPos newVarPos(Variable v, VarPos prev, VarPos next) {
	VarPos new = safeMalloc(sizeof(varPos));
	new->var = v;
	new->prev = prev;
	new->next = next;
	if(prev != NULL) {
		prev->next = new;
	}
	if(next != NULL) {
		next->prev = new;
	}
	return new;
}

/*
 * 
*/ 
void freeVarSeqRec(VarPos current) {
	if(current != NULL) {
		freeVarSeqRec(current->next);
		free(current);
	}
}

/*
 * 
*/ 
void freeVarSeq(VarSeq sequence) {
	if(sequence != NULL) {
		freeVarSeqRec(sequence->first);
		free(sequence);
	}
}

int countVarSeq(VarSeq sequence) {
  VarPos pos = sequence->first;
  int cnt = 0;
  while(pos != NULL) {
    cnt++;
    pos = pos->next;
  }
  return cnt;
}


/*
 * 
*/ 
void removeVarFromSequence(VarSeq sequence, VarPos position) {
  int cnt = countVarSeq(sequence);

  if(!varInSequence(position->var, sequence)) {
    return;
  }

	/* 'knot' previous prev and next */
	if(position->prev != NULL) {
		position->prev->next = position->next;
	} else {	/* position is first position */
		sequence->first = position->next;
	}
	if(position->next != NULL) {
		position->next->prev = position->prev;
	}
	position->next = NULL;
	position->prev = NULL;
	assert(cnt == countVarSeq(sequence)+1);
}

/*
 * 
*/ 
int varInSequence(Variable v, VarSeq sequence) {
	VarPos position = sequencePosition(v);
	if(position == NULL) {
		return 0;
	}
	if(position->prev != NULL || position->next != NULL) {
		return 1;
	}
	return (sequence->first == position);
}

/*
 * Inserts a variable v in sequence of variables 
*/ 
VarPos insertVarInSequence(VarSeq sequence, Variable v) {
	/* insert at first position */
	sequence->first = newVarPos(v, NULL, sequence->first); 
	/* tell variable it's new position */
	setSeqPosOfVar(v, sequence->first);
	/* resort sequence */
	resortVarSeq(sequence, sequence->first);
	return sequencePosition(v);
}

void insertAfter(VarSeq seq, VarPos pos, VarPos prev) {
	/* set new neighbours */
	pos->prev = prev;
	
	/* tell new neighbours you are living here */
	if(pos->prev != NULL) {
		pos->next = pos->prev->next;
		pos->prev->next = pos;
	} else {
	  pos->next = seq->first;
		seq->first = pos;
	}
	if(pos->next != NULL) {
		pos->next->prev = pos;
	}
}

/* resorts variable at VarPos in the VarSequence of variables */
void resortVarSeq(VarSeq sequence, VarPos position) {
	VarPos next = position->next;
	VarPos prev = position->prev;
	/* shift to right as long as needed */
	while(next != NULL && !varOrdered(position->var, next->var)) { 	
		prev = next;
		next = next->next;
	}
	/* shift to left as long as needed */
	while(prev != NULL && !varOrdered(prev->var, position->var)) {	
		next = prev;
		prev = prev->prev;
	}
	/* remove at current position */
	removeVarFromSequence(sequence, position);			
	/* insert at new position */		
	
	insertAfter(sequence, position, prev);
}


void assertSequenceSorted(VarSeq sequence) {
  VarPos first = sequence->first;
  VarPos second = first->next;
  while(second != NULL) {
    assert(varOrdered(first->var, second->var));
    first = second;
    second = second->next;
  }
}


/* 
 * Function that restores the sequence to an earlier version
 * given a backup of a position of a variable in the sequence
*/
void restoreVarSeq(VarSeq sequence, varPos backup) {
  int cnt = countVarSeq(sequence);
  int inSeq = varInSequence(backup.var, sequence);
	/* remove variable from current position */
	removeVarFromSequence(sequence, backup.var->sequencePos);

	/* insert at new position */
	insertAfter(sequence, backup.var->sequencePos, backup.prev);
	resortVarSeq(sequence, backup.var->sequencePos);
	assert(countVarSeq(sequence) == cnt + !inSeq);
}

/* 
 * Function that returns the first VarPos, given a VarSequence
*/
VarPos firstVarPosition(VarSeq sequence) {
	return sequence->first;
}

/*
 * Given a sequence and position in sequence 
 * -> return var at that position
 * VarSequence as argument because abstraction 
 * (implementation could change)
*/
Variable varAtPosition(VarSeq seq, VarPos pos) {
	if(pos == NULL) {
		return NULL;
	}
	return pos->var;
}

/* 
 * This function prints, given a VarSequence, for each variable
 * - the domains of the variable
 * - the degree (constraints with unassigned variables) of the variable
*/
void printVarSequence(VarSeq sequence) {
	VarPos pos = sequence->first;
	addLog("BEGIN Sequence of variables: \n");	
	while(pos != NULL) {
		printVar(pos->var);
		pos = pos->next;
	}
	addLog("END Sequence of variables: \n");
}

/* 
 * Checks if variable first should be selected before variable second
 * when MRV (without degree heuristic as a tie-breaker) is applied
*/
int mrvOrdered(Variable first, Variable second) {
	int domSize1 = domainSizeOfVar(first);
	int domSize2 = domainSizeOfVar(second);
	return (domSize1 <= domSize2);	
}

/* 
 * Checks if variable first should be selected before variable second
 * when MRV + degree heuristic is applied
*/
int mrvPlusDegreeOrdered(Variable first, Variable second) {  
	int domSizeFirst = domainSizeOfVar(first);
	int domSizeSecond = domainSizeOfVar(second);
	if(domSizeFirst < domSizeSecond) {
		return 1;
	} else if(domSizeFirst == domSizeSecond) {
		return (first->constraintDegree >= second->constraintDegree);
	}
	return 0;
}

int mostConnectedOrdered(Variable first, Variable second) {
  /*return (first->connectivity >= second->connectivity);*/
  double connectivity1 = first->connectivity;
  double connectivity2 = second->connectivity;
  /*double conns1 = first->varConnections  
  double conns2 = second->varConnections 
  connectivity1 /= conns1;
  connectivity2 /= conns2;*/
  return (connectivity1 >= connectivity2);
}

int mrvPlusConnectedOrdered(Variable first, Variable second) {  
	int domSizeFirst = domainSizeOfVar(first);
	int domSizeSecond = domainSizeOfVar(second);
	if(domSizeFirst < domSizeSecond) {
		return 1;
	} else if(domSizeFirst == domSizeSecond) {
		return mostConnectedOrdered(first, second);
	}
	return 0;
}


/* 
 * Function that always agrees with order of variables first and second
 * in variable sequence
*/
int skipTest(Variable first, Variable second) {
	return 1;
}

/* END Functions VarSequence */


int nodeReduce(Variable v, Constraint c, Problem p) {
	IntegerSet domain = domainOfVar(v);
	int *values = valuesOfSet(domain);
	int changed = 0;
	int remove, i;
	if(arityOfConstraint(c) != 1) {
		return 0;
	}
	for(i = 0; i < sizeOfSet(domain); i++) {
		setDomainOfVar(v, createSingletonDomain(values[i]));
		remove = !checkConstraint(c, p);
		freeIntegerSet(domainOfVar(v));
		if(remove) {
			changed = 1;
			removeNthIntegerFromSet(domain, i);
			i--;
		}
	}
	setDomainOfVar(v, domain);
	return changed;
}

int arcReduce(DirectedArc arc, Problem p) {
  Variable v = firstVarOfArc(arc);
  Constraint c = constraintOfArc(arc);
	IntegerSet domain = domainOfVar(v);
	int *values = valuesOfSet(domain);
	int changed = 0;
	int remove, i;
	/* arity of constraint must be 2 (binary, in case of arc consistency check) or
	    arity must be 1 (after assignment var and forwardchecking for all arcs directed at var) */
	assert(arityOfConstraint(c) <= 2);
	for(i = 0; i < sizeOfSet(domain); i++) {
		setDomainOfVar(v, createSingletonDomain(values[i]));
		remove = !satisfiable(c, p);
		freeIntegerSet(domainOfVar(v));
		if(remove) {
			changed = 1;
			removeNthIntegerFromSet(domain, i);
			i--;
		}
	}
	setDomainOfVar(v, domain);
	return changed;
}

void plusVarConnections(Variable v) {
  v->varConnections++;
}




