#ifndef SOLVE_H
#define SOLVE_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "variable.h"
#include "problem.h"
#include <stdarg.h>

typedef struct solutionSet *SolutionSet;
typedef struct solutionList *SolutionList;

typedef struct solutionList {
	int *values;
	SolutionList next;
} solutionList;

typedef struct solutionSet {
	SolutionList first;
	SolutionList last;
	int varAmount;
	int solutionSpace;
	int solutionCount;
} solutionSet;

SolutionSet newSolutionSet(int varAmount, int solutionSpace);
void addSolution(SolutionSet solset, Problem p);
int solutionsLeft(SolutionSet solset);
void freeSolutionSet(SolutionSet solset);
void printSolution(int *solution, int varCount);

void addLog(const char * format, ...);
int init(Problem p);
Problem backtrack(Problem p);
SolutionSet solve(Problem p);
void enableHeuristics(int argc, char **argv);


#endif
