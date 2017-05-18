#ifndef PROBLEM_H
#define PROBLEM_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct problem *Problem;
typedef struct directedArc *DirectedArc;

#include "variable.h"
#include "constraint.h"

typedef enum {
	SOLVEALL, SOLVENR
} SolveType;

typedef struct SolveSpec {
	SolveType type;
	int max;
} SolveSpec;

typedef struct directedArc {
  Variable from;
  Constraint constraint;
  Variable to;
} directedArc;

typedef struct problem {
	int varCount;				/* nr of variables for this Problem */
	int assignCount;			/* nr of variables assigned */
	int constraintCount;		/* nr of constraints for this Problem */
	Variable *vars;				/* array of variables for this Problem */
	VarSeq varSequence;
	Constraint *constraints;	/* array of constraints for this Problem */
	SolveSpec solvespec;		/* how many solutions should be determined */
} problem;

void setVarsOfProblem(Problem p, VarList vl);
void setConstraintsOfProblem(Problem p, ConstraintList constraints);
void setDomainsOfVars(Problem p, VarList varIndices, IntegerSet d);

Variable varByIndex(Problem p, int index);
void setVarAtIndex(Problem p, int index, Variable v);
Constraint constraintByIndex(Problem p, int index);
void setConstraintAtIndex(Problem p, int index, Constraint c);
VarSeq varSeqOfProblem(Problem p);

void printProblem(Problem p);
/*void freeProblemVars(Problem p);*/
Problem emptyProblem();
void freeProblem(Problem p);



DirectedArc makeArc(Variable from, Constraint c, Variable to);
void freeArc(DirectedArc arc);
Variable firstVarOfArc(DirectedArc arc);
Variable secondVarOfArc(DirectedArc arc);
Constraint constraintOfArc(DirectedArc arc);



#endif
