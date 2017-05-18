#ifndef VARIABLE_H
#define VARIABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct variable *Variable;
typedef struct domainsBackupList *DomainsBackupList;
typedef struct domainsBackup *DomainsBackup;
typedef struct varList *VarList;
typedef struct varPos *VarPos;
typedef struct varSeq *VarSeq;
typedef struct variable *Variable;

#include "datatypes.h"
#include "constraint.h"


typedef struct variable {
	int index;
	int constraintDegree;
	int connectivity;
	int varConnections;
	DataType type;
	IntegerSet domain;
	IntegerSet constraints;
	VarPos sequencePos;
} variable;

typedef struct varList {
	int varIndex;
	DataType varType;
	VarList next;
} varList;

typedef struct varPos {
	Variable var;
	VarPos prev, next;
} varPos;

typedef struct varSeq {
	VarPos first;
} varSeq;


Variable newVariable(int varIndex, DataType d);
void setDomainOfVar(Variable v, IntegerSet domain);
void freeVariable(Variable v);
void addConstraintToVar(Variable var, Constraint c);
void removeConstraintFromVar(Variable v, Constraint c);

int indexOfVar(Variable v);
DataType dataTypeOfVar(Variable v);
int constraintAmountOfVar(Variable v);
int *constraintIndicesOfVar(Variable v);
int isAssigned(Variable v, Problem p);
int varInSequence(Variable v, VarSeq sequence);
IntegerSet domainOfVar(Variable v);
VarPos sequencePosition(Variable v);
void lowerDegree(Variable v);
void plusDegree(Variable v);
int degreeOfVar(Variable v);
void lowerConnectivity(Variable v);
void plusConnectivity(Variable v);

IntegerSet createSingletonDomain(int value);
int domainSizeOfVar(Variable v);
void printDomainOfVar(Variable v);
int singletonDomain(IntegerSet d);
int *domainValuesOfVar(Variable v);
int domainMinimumOfVar(Variable v);
int domainMaximumOfVar(Variable v);

VarList newVarList(int varIndex, VarList next);
void freeVarList(VarList vl);
void addVar(VarList vl, int varIndex);
VarList addVars(VarList vl, DataType d, VarList toAdd);
Variable *varListToArray(VarList vl);

void printVar(Variable v);


VarSeq emptyVarSeq();
void freeVarSeq(VarSeq sequence);
void restoreVarSeq(VarSeq sequence, varPos backup);
void resortVarSeq(VarSeq sequence, VarPos position);

void assertSequenceSorted(VarSeq sequence);

VarPos insertVarInSequence(VarSeq sequence, Variable v);
void removeVarFromSequence(VarSeq sequence, VarPos position);
VarPos firstVarPosition(VarSeq sequence);
Variable varAtPosition(VarSeq sequence, VarPos position);
void printVarSequence(VarSeq sequence);


int mrvPlusDegreeOrdered(Variable first, Variable second);
int mrvOrdered(Variable first, Variable second);
int mostConnectedOrdered(Variable first, Variable second);
int mrvPlusConnectedOrdered(Variable first, Variable second);
int skipTest(Variable first, Variable second);

int nodeReduce(Variable v, Constraint c, Problem p);
int arcReduce(DirectedArc arc, Problem p);

void plusVarConnections(Variable v);

#endif
