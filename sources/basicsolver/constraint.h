#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h> 

struct Constraint;
typedef struct value *Value;
typedef struct term *Term;
typedef struct numExp *NumExp;
typedef struct factor *Factor;
typedef struct functionCall *FunctionCall;
typedef struct constraintList *ConstraintList;
typedef struct constraint *Constraint;

#include "variable.h"
#include "problem.h"



typedef enum {
	NUMNEG, VALUE
} FactorType;

typedef enum {
	INTVAL, VARVAL, FUNCVAL, NUMEXP
} ValueType;

typedef enum {
	MAX, MIN, ABS, ANY, ALL
} FunctionName;

typedef enum Rel{
	IS, SMALLER, GREATER, GEQ, LEQ, NEQ
} RelOperator;

typedef enum {
	MINUS, PLUS
} TermOperator;

typedef enum {
	MUL, DIV, MOD
} FactorOperator;


typedef union {
	int intval;
	FunctionCall funcCall;
	int varIndex;
	NumExp numexp;
} ValueData;

typedef struct value {
	ValueType type;
	ValueData data;
	Factor exponent;
} value;

typedef union {
	Factor factor;
	Value value;
} FactorData;

typedef struct factor {
	FactorType type;
	FactorData data;
} factor;


typedef struct functionCall {
	FunctionName name;
	int argc;
	void *argv;
} functionCall;

typedef struct term {
	void *data;
	Term next;
	FactorOperator factorop;
} term;

typedef struct numExp {
	void *data;
	NumExp next;
	TermOperator termop;
} numExp;


typedef struct constraint {		/* exp1 [op exp2]? */
	int index;
	int arity;
	IntegerSet vars;
	NumExp exp1;
	
	/* optional: if exp2 != NULL. constraint: exp1 [op exp2]? */
	RelOperator op;			
	NumExp exp2;		
} constraint;

typedef struct constraintList {
	Constraint constraint;
	ConstraintList next;
} constraintList;

Constraint newValConstraint(NumExp e1);
Constraint newBoolConstraint(NumExp e1, RelOperator o, NumExp e2);

void setOperatorOfConstraint(Constraint c, RelOperator relop);
void setSecondExp(Constraint c, NumExp exp2);
void setIndexOfConstraint(Constraint c, int index);
void addVarToConstraint(Constraint c, Variable v);
void removeVarFromConstraint(Constraint c, Variable v);

NumExp firstExp(Constraint c);
NumExp secondExp(Constraint c);
RelOperator operatorOfConstraint(Constraint c);
int indexOfConstraint(Constraint c);

void freeConstraint(Constraint c);
void printConstraint(Constraint c);

ConstraintList addConstraint(ConstraintList list, Constraint constraint);
void freeConstraintList(ConstraintList cl);
void printConstraintList(ConstraintList cl);
int *varIndicesOfConstraint(Constraint c);

NumExp newNumExp(void *data);
NumExp addTerm(NumExp old, TermOperator termop, Term t);
void freeNumExp(NumExp numexp);
void printNumExp(NumExp numExp);
void printRelOperator(RelOperator op);

Term newTerm(void *data);
Term addFactor(Term old, FactorOperator factorop, Factor f);
void freeTerm(Term t);
void printTerm(Term t);
void printTermOperator(TermOperator op);
int singletonTerm(Term t, Problem p);

Factor newFactor(FactorType type);
Factor newValueFactor(Value v);
Factor newMinusFactor(Factor next);
void freeFactor(Factor f);
void printFactor(Factor f);
void printFactorOperator(FactorOperator op);
int singletonFactor(Factor f, Problem p);
FactorType typeOfFactor(Factor f);
Factor subFactorOfFactor(Factor f);
Value subValueOfFactor(Factor f);

FunctionCall newFunctionCall(FunctionName name, int argc, void *argv);
FunctionName nameOfFuncCall(FunctionCall fc);
int argCountOfFunctionCall(FunctionCall fc);
void *argumentsOfFuncCall(FunctionCall fc);
void freeFunctionCall(FunctionCall f);

Value newValue(ValueType type);
Value newIntVal(int intval);
Value newVarVal(int varIndex);
Value newFuncVal(FunctionCall fc);
Value newNumExpVal(NumExp numexp);
void freeValue(Value v);
void printValue(Value v);
int singletonValue(Value v, Problem p);


Term newTerm(void *data);
Term addFactor(Term old, FactorOperator factorop, Factor f);
void freeTerm(Term t);
void printTerm(Term t);

NumExp newNumExp(void *data);
NumExp addTerm(NumExp numExp, TermOperator termop, Term t);
void freeNumExp(NumExp numexp);
int singletonNumExp(NumExp n, Problem p);
int matchVarExpression(Variable v, NumExp exp);


/* Functions regarding calculation expressions */
int max(NumExp exp1, NumExp exp2, Problem p);
int min(NumExp exp1, NumExp exp2, Problem p);
int absVal(NumExp exp1, Problem p);
int calcFuncVal(FunctionCall fc, Problem p);
int calcValue(Value v, Problem p);
int calcFactor(Factor f, Problem p);
int calcTerm(Term t, Problem p);
int calcExp(NumExp exp, Problem p);

/* Functions regarding properties of constraints */
int arityOfConstraint(Constraint c);
int determinable(Constraint c, Problem p);

/*Constraint replaceVar(Constraint c, Variable v);*/

/* Functions regarding truth of constraints */
int satisfiable(Constraint c, Problem p);
int or(ConstraintList list, Problem p);
int and(ConstraintList list, Problem p);
RelOperator switchOperator(RelOperator relop);
int checkRelation(int left, RelOperator relop, int right);
int checkConstraint(Constraint c, Problem p);



#endif
