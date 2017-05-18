#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "datatypes.h"
#include <limits.h>
#include "datatypes.h"
#include "problem.h"
#include "variable.h"
#include "constraint.h"
#include "solve.h"

extern FILE *logFile;

static void setConstraintOfNumExp(NumExp exp, Constraint c, Problem p);
static void setConstraintOfFactor(Factor exp, Constraint c, Problem p);


static void setConstraintOfValue(Value v, Constraint c, Problem p) {
	if(v->type == VARVAL) {
	  Variable var = varByIndex(p, v->data.varIndex);
		addConstraintToVar(var, c);
		addVarToConstraint(c, var);
	} else if(v->type == NUMEXP) {
		setConstraintOfNumExp(v->data.numexp, c, p);
	} else if(v->type == FUNCVAL) {
		int i;
		if(v->data.funcCall->name == ANY || v->data.funcCall->name == ALL) {
			ConstraintList *lists = v->data.funcCall->argv;
			ConstraintList l = lists[0];
			while(l != NULL) {
				setConstraintOfNumExp(firstExp(l->constraint), c, p);
				if(secondExp(l->constraint) != NULL) {
					setConstraintOfNumExp(secondExp(l->constraint), c, p);
				}
				l = l->next;
			}
		} else {
			NumExp *numExps = v->data.funcCall->argv;
			for(i = 0; i < v->data.funcCall->argc; i++) {
				setConstraintOfNumExp(numExps[i], c, p);
			}
		}
	}
	if(v->exponent != NULL) {
		setConstraintOfFactor(v->exponent, c, p);
	}
}

static void setConstraintOfFactor(Factor f, Constraint c, Problem p) {
	if(f->type == NUMNEG) {
		setConstraintOfFactor(f->data.factor, c, p);
	} else {
		setConstraintOfValue(f->data.value, c, p);
	}
}

static void setConstraintOfTerm(Term t, Constraint c, Problem p) {
	if(t->next == NULL) {
		setConstraintOfFactor(t->data, c, p);
	} else {
		setConstraintOfTerm(t->data, c, p);
		setConstraintOfTerm(t->next, c, p);
	}
}

static void setConstraintOfNumExp(NumExp exp, Constraint c, Problem p) {
	if(exp->next == NULL) {
		setConstraintOfTerm(exp->data, c, p);
	} else {
		setConstraintOfNumExp(exp->data, c, p);
		setConstraintOfNumExp(exp->next, c, p);
	}
}

static void freeProblemVars(Problem p) {
	int i;
	for(i = 0; i < p->varCount; i++) {
		freeVariable(p->vars[i]);
	}
	free(p->vars);
}

static void freeProblemConstraints(Problem p) {
	int i;
	for(i = 0; i < p->constraintCount; i++) {
		freeConstraint(p->constraints[i]);
	}
	free(p->constraints);
}

void linkConstraintsAndVars(Problem p) {
	int i;
	for(i = 0; i < p->constraintCount; i++) {
	  Constraint c = constraintByIndex(p, i);
		setConstraintOfNumExp(firstExp(c), c, p);
		if(secondExp(p->constraints[i]) != NULL) {
			setConstraintOfNumExp(secondExp(c), c, p);
		}
	}
}

Problem emptyProblem() {
	Problem p = safeMalloc(sizeof(problem));
	p->varCount = 0;
	p->assignCount = 0;
	p->constraintCount = 0;
	/*p->connected = NULL;
	p->hasConstraint = NULL;*/
	p->vars = NULL;
	p->varSequence = NULL;
	p->constraints = NULL;
	p->solvespec.type = SOLVENR;
	p->solvespec.max = 1;
	return p;
}


void freeProblem(Problem p) {
	freeProblemVars(p);
	freeProblemConstraints(p);
	freeVarSeq(p->varSequence);
	free(p);
}


VarSeq varSeqOfProblem(Problem p) {
	return p->varSequence;
}


DirectedArc makeArc(Variable from, Constraint c, Variable to) {
  DirectedArc arc = safeMalloc(sizeof(directedArc));
  arc->from = from;
  arc->constraint = c;
  arc->to = to;
  return arc;
}

void freeArc(DirectedArc arc) {
  free(arc);
}

Variable firstVarOfArc(DirectedArc arc) {
  return arc->from;
}

Variable secondVarOfArc(DirectedArc arc) {
  return arc->to;
}

Constraint constraintOfArc(DirectedArc arc) {
  return arc->constraint;
}


void setVarsOfProblem(Problem p, VarList vl) {
	int i;
  Variable *vars;
	VarList tmp = vl;
	int varCount = 0;
	while(tmp != NULL) {
		varCount++;
		tmp = tmp->next;
	}
	vars = safeMalloc(varCount * sizeof(Variable));
	tmp = vl;
	for(i = 0; i < varCount; i++) {
		vars[i] = newVariable(tmp->varIndex, tmp->varType);
		tmp = tmp->next;
	}
	freeVarList(vl);
	p->varCount = varCount;
	p->vars = vars;
}

void setDomainsOfVars(Problem p, VarList toSet, IntegerSet d) {
	while(toSet != NULL) {
		freeIntegerSet(domainOfVar(p->vars[toSet->varIndex]));
		setDomainOfVar(p->vars[toSet->varIndex], copyIntegerSet(d));
		toSet = toSet->next;
	}
}

void setConstraintsOfProblem(Problem p, ConstraintList cl) {
	int i;
	ConstraintList tmp = cl;
	p->constraintCount = 0;
	while(tmp != NULL) {
		p->constraintCount++;
		tmp = tmp->next;
	}
	
	p->constraints = safeMalloc(p->constraintCount * sizeof(Constraint));
	tmp = cl;
	for(i = 0; tmp != NULL; i++) {
		p->constraints[i] = tmp->constraint;
		setIndexOfConstraint(tmp->constraint, i);
		tmp = tmp->next;
	}
	
	freeConstraintList(cl);
	linkConstraintsAndVars(p);	
}

Variable varByIndex(Problem p, int index) {
	return p->vars[index];
}

void setVarAtIndex(Problem p, int index, Variable v) {
  p->vars[index] = v;
}

Constraint constraintByIndex(Problem p, int index) { 
	return p->constraints[index];
}

void setConstraintAtIndex(Problem p, int index, Constraint c) {
  p->constraints[index] = c;
}


