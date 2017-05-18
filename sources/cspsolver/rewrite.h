#ifndef REWRITE_H
#define REWRITE_H

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "vardb.h"

typedef struct sizedArray {
	int size;
	int *array;
} sizedArray;

void printComma(int boolean);

void rewriteVarDef(Variable var, int comma);
/*void rewriteVarDefList(List vl);*/
void rewriteVarCall(VarCall vc, List substset, int domainset);
void rewriteVarList(List vl, List substset, int domainset);
void rewriteDomain(List subdoms, List substset);
void rewriteDomainSpec(RecursiveType domspec, List substset);

void rewriteAllDiff(List varcalls, List substset);


void rewriteNumExp(NumExp, List substset);
void rewriteVarSubstCall(VarCall vc, List substset);
void rewriteFunctionCall(FunctionCall fc, List substset);
void rewriteFactor(Factor f, List substset);
void rewriteValue(Value v, List substset);
void rewriteTerm(Term t, List substset);
void rewriteConstraint(Constraint c, List substset);

int calcConstraint(Constraint c, List substset);
int calcNumExp(NumExp exp, List substset);
int calcTerm(Term exp, List substset);
int calcFactor(Factor exp, List substset);
int calcValue(Value exp, List substset);
int calcFunctionCall(FunctionCall exp, List substset);


void rewriteConstraintSpec(RecursiveType cs, List substset);
void printNFVar(int idx);


#endif
