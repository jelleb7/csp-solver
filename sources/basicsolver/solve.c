#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h> 
#include <sys/resource.h>
#include "datatypes.h"
#include "problem.h"
#include "variable.h"
#include "constraint.h"
#include "backup.h"
#include "solve.h"

#define ON 1
#define OFF 0
#define NODE 1
#define ARC 2
#define FC 1
#define MAC 2

#define LOG ON

/* which techniques should be applied while solving CSP */
int MAKECONSISTENT = OFF;   	/* OFF/NODE/ARC */
int MRV = OFF;   				/* ON/OFF */
int DEGREE_HEURISTIC = OFF;   	/* ON/OFF */
int MOSTCONNECTED = OFF;   		/* ON/OFF */
int CP = OFF;   				/* OFF/FC/MAC */

int match(char *str1, char *str2) {
	return !strcmp(str1, str2);
}

void enableHeuristics(int argc, char **argv) {
	int fcset = 0;
	int arcset = 0;
	for(int i = 0; i < argc; i++) {
		if(match(argv[i], "-iconst")) {
			if(argc-1 < i+1) {
				fprintf(stderr, "[ERROR] Expecting a number following flag '-iconst'.\n" );
				exit(1);
			}
			// match number following
			int k = atoi(argv[i+1]);
			if(k >= 0 && k < 3) {
				MAKECONSISTENT = k;
			} else {
				fprintf(stderr, "[ERROR] Expecting number in [0..2] following flag '-iconst'.\n" );
				exit(1) ;
			}
			i++;
			continue;
		}
		if(match(argv[i], "-mrv")) {
			MRV = ON;
			continue;
		} 
		if(match(argv[i], "-deg")) {
			DEGREE_HEURISTIC = ON;
			continue;
		} 
		if(match(argv[i], "-mostconnected")) {
			MOSTCONNECTED = ON;
			continue;
		}
		if(match(argv[i], "-fc")) {
			if(arcset) {
				fprintf(stderr, "[ERROR] -fc and -arc not allowed together.\n" );
				exit(1) ;
			}
			CP = FC;
			fcset = 1;
			continue;
		}
		if(match(argv[i], "-arc")) {
			if(fcset) {
				fprintf(stderr, "[ERROR] -fc and -arc not allowed together.\n" );
				exit(1) ;
			}
			CP = MAC;
			arcset = 1;
			continue;
		}
		char errstr[strlen(argv[i])+30];
		strcpy(errstr, "[ERROR] Unknown argument '");
		strcat(errstr, argv[i]);
		strcat(errstr, "'.\n");
		fprintf(stderr, "%s", errstr);
		exit(-1);
	}
	
}

/* function pointers that are set based on the applied techniques/heuristics */
int (*varOrdered)(Variable, Variable);
int (*propagationSuccess)(Variable, Problem, Backup);
static int **inArcsQueue;

/* contains file descriptor of logFile: also referred to in other files */
extern FILE *logFile;

int initdone = 0;

/* counts the amount of backtracking calls */
long stateCount = 0;

/* adds content to the logFile, if logging is enabled */
/* src: http://www.cplusplus.com/reference/cstdio/vprintf/ */
void addLog(const char * format, ...) {
	if(LOG) {
		va_list args;
  	va_start (args, format);
  	vfprintf (logFile, format, args);
  	va_end (args);
  }
}

/* 
	This function adds all directed arcs (X --> var) to a given queue
	an arc is unique by X, var and the constraint that connects these two
	(can be more than 1 constraint, so more than one arc representing (X --> var)
*/
void addVariableArcs(Variable var, Queue arcQueue, Problem p) {
	int i;
	int *varIndices;
	
	/* get indices of constraints in which var is involved */
	int *constraintIndices = constraintIndicesOfVar(var);				
	int varAssigned = isAssigned(var, p);
	Constraint c;
	
	/* for all these constraints */
	for(i = 0; i < constraintAmountOfVar(var); i++) {	
		c = constraintByIndex(p, constraintIndices[i]);
		
		/* if constraint is binary */
		if(arityOfConstraint(c) == 1 + (!varAssigned || !initdone)) {           
			varIndices = varIndicesOfConstraint(c);	
			
			/* var2 becomes other variable in binary constraint */
		  Variable var2 = varByIndex(p, varIndices[0]);
			if(var2 == var) {
				var2 = varByIndex(p, varIndices[1]);
			}
			
			/* if (constraint, var2) representing directed arc not already in queue */
			if(!inArcsQueue[c->index][var2->index]) {				
			  /* enqueue directed arc (var2 --> var) */			
			  DirectedArc arc = makeArc(var2, c, var);
			  enqueue(arcQueue, arc);                     
			  
			          
			  /* mark this arc as available in the queue */
				inArcsQueue[indexOfConstraint(c)][indexOfVar(var2)] = 1;						  
			}
		}
	}
}

/* 
	This function adds all constraints for var with arity 3 that became binary 
	after assigning var to a given queue.
*/
void addNewArcs(Variable var, Queue arcQueue, Problem p) {
	int i;
	int *varIndices;
	
	/* get indices of constraints in which var is involved */
	int *constraintIndices = constraintIndicesOfVar(var);				
	Constraint c;
	
	/* for all these constraints */
	for(i = 0; i < constraintAmountOfVar(var); i++) {	
	  c = constraintByIndex(p, constraintIndices[i]);
		
		/* if constraint is binary */
		if(arityOfConstraint(c) == 2) {           
			varIndices = varIndicesOfConstraint(c);	
			
			/* var2, var3 becomes variable left in (now) binary constraint */
		  Variable var2 = varByIndex(p, varIndices[0]);
		  Variable var3 = varByIndex(p, varIndices[1]);
			
			/* if (constraint, var2) representing directed arc not already in queue */
			if(!inArcsQueue[c->index][var2->index]) {				
			  /* enqueue directed arc (var2 --> var) */			
			  DirectedArc arc = makeArc(var2, c, var);
			  enqueue(arcQueue, arc);                     
			          
			  /* mark this arc as available in the queue */
				inArcsQueue[indexOfConstraint(c)][indexOfVar(var2)] = 1;						  
			}
			if(!inArcsQueue[c->index][var3->index]) {				
			  /* enqueue directed arc (var3 --> var) */			
			  DirectedArc arc = makeArc(var3, c, var);
			  enqueue(arcQueue, arc);                     
			          
			  /* mark this arc as available in the queue */
				inArcsQueue[indexOfConstraint(c)][indexOfVar(var3)] = 1;						  
			}
		}
	}
}

/*
	This function ensures that all variables of CSP p are consistent 
	regarding their unary constraints. 
	If a value in the domain of a variable is not consistent 
	with one of the unary constraints it is deleted. 
	When all inconsistent values are removed, the constraint is also removed, 
	such that it is not checked redundantly later on in the process.
*/
void makeNodeConsistent(Problem p) {
	int i;
	int *varIndices;
	Constraint constraint;
	Variable var;
	
	/* for each constraint of CSP p */
	for(i = 0; i < p->constraintCount; i++) {					
		constraint = p->constraints[i];
		/* if constraint is unary */		
		if(arityOfConstraint(constraint) == 1) {				
		  /* get indices of variables occurring in constraint */
		  varIndices = varIndicesOfConstraint(constraint);	
		  /* get variable which is involved in this constraint */
			var = varByIndex(p, varIndices[0]);		
			/* if domain of variable var is reduced */				
			if(nodeReduce(var, constraint, p)) {						
			  /* if domain of var became empty -> error, no solution exists */
				if(domainSizeOfVar(var) == 0) {						
					addLog(
					  "No solutions found for the problem, \
					  because variable X%d has an empty domain\n", 
					  var->index
					);
					printf(
					  "\nNo solutions found for the problem, \
					  because variable with empty domain\n"
					);
					exit(-1);
				}
			}
			/* remove unary constraint so that it is no longer checked */
			removeConstraintFromVar(var, constraint);				
		}
	}
}

void printVarDomains(Problem p) {
	int i;
	for(i = 0; i < p->varCount; i++) {
		addLog("X%d: ", i);
		printDomainOfVar(varByIndex(p, i));
		addLog("\n");
	}
}

/*
	This function ensures that all domains of the variables of CSP are consistent 
	regarding their binary constraints. After execution
	the CSP p is fully arc-consistent or strongly 2-consistent.
*/
void makeArcConsistent(Problem p) {
	int i;
	int *varIndices;
	Queue arcQueue = emptyQueue();
	
	/* first the CSP is made node-consistent */
	makeNodeConsistent(p);	
	
	/* for each constraint of p */	
	for(i = 0; i < p->constraintCount; i++) {					
		Constraint constraint = constraintByIndex(p, i);
		/* get indices of variables occurring in constraint */
		varIndices = varIndicesOfConstraint(constraint);			
		/* if constraint is a binary constraint */
		if(arityOfConstraint(constraint) == 2) {		
		  Variable var1 = varByIndex(p, varIndices[0]);
		  Variable var2 = varByIndex(p, varIndices[1]);
		  
		  /* enqueue directed arc (var1 --> var2) */
			enqueue(arcQueue, makeArc(var1, constraint, var2));	 	
			/* enqueue directed arc (var2 --> var1) */
			enqueue(arcQueue, makeArc(var2, constraint, var1));	 	
			
			/* directional arc (var1 --> var2) is in queue */
			inArcsQueue[constraint->index][varIndices[0]] = 1;		
			/* directional arc (var2 --> var1) is in queue */
			inArcsQueue[constraint->index][varIndices[1]] = 1;
		}
	}
	
	/* until queue is empty */
	while(!isEmptyQueue(arcQueue)) {							
		DirectedArc arc = dequeue(arcQueue);
		Variable var1 = firstVarOfArc(arc);
		Constraint c = constraintOfArc(arc);
		/* arc is not in queue anymore */	
		inArcsQueue[indexOfConstraint(c)][indexOfVar(var1)] = 0;				
		
		if(arcReduce(arc, p)) {		/* if domain reduction */
		  /* if domain of var became empty -> error, no solution exists */
			if(domainSizeOfVar(var1) == 0) {					
				addLog(
				  "No solutions found for the problem, \
				  because variable X%d has an empty domain\n", 
				  indexOfVar(var1)
				);
				printf(
				  "\nNo solutions found for the problem, \
				  because variable with empty domain\n"
				);
				exit(-1);
			}
			/* because domain is reduced, domains of neighbours might be reduced */
			addVariableArcs(var1, arcQueue, p);	
			
		}
		freeArc(arc);
	}
	
	freeQueue(arcQueue);
}

/* 
  Checks if the assignment to variable var is consistent 
  with current partial assignment.
*/
int checkLocalConsistency(Variable var, Problem p) {
	int i;
	int *constraintIndices = constraintIndicesOfVar(var);
	Constraint c;
	
	/* for all constraints connected to variable var */
	for(i = 0; i < constraintAmountOfVar(var); i++) {		
		c = constraintByIndex(p, constraintIndices[i]);				
		/* check if inconsistent with partial solution */
		if(determinable(c, p) && !checkConstraint(c, p)) {		
			return 0;
		}
	}
	return 1;
}

/* 
  This function checks if the current partial solution after assignment of var
  is consistent and (if techniques applied) removes invalid values from domains
  of unassigned variables by constraint propagation
*/
int isConsistent(Variable var, Problem p, Backup b) {
  if(checkLocalConsistency(var, p)) {
    return propagationSuccess(var, p, b);
  }
  return 0;
}

/* 
  Function that makes the queue empty and sets all arcs as 'not available'
  for queue containing directed arcs.
*/
void makeArcQueueEmpty(Queue arcQueue) {
  /* for all arcs in queue */
  while(!isEmptyQueue(arcQueue)) {
    DirectedArc arc = dequeue(arcQueue);
    Constraint c = constraintOfArc(arc);
    Variable var1 = firstVarOfArc(arc);
    /* set arc (var1 --> otherVar) by constraint c 'not available' */
    inArcsQueue[indexOfConstraint(c)][indexOfVar(var1)] = 0;	
    freeArc(arc);
  }
}

/* 
  Function that can be used as placeholder if no constraint propagation
  is applied.
*/  
int skipPropagationTest(Variable var, Problem p, Backup backup) {
  return 1;
}

int forwardChecking(Variable var, Problem p, Backup backup) {
  Queue arcQueue = emptyQueue();
	
	addLog("forward checking based on assignment of variable X%d\n", var->index);
	
	/* enqueue all arcs directed at var, so all (X --> var) */
  addVariableArcs(var, arcQueue, p);
  
  /* for all arcs in queue */
  while(!isEmptyQueue(arcQueue)) {		
    /* dequeue arc */					
    DirectedArc arc = dequeue(arcQueue);
    Constraint c = constraintOfArc(arc);
    Variable var1 = firstVarOfArc(arc);
    
    /* set arc 'not available' */
    inArcsQueue[indexOfConstraint(c)][indexOfVar(var1)] = 0;		
		
		/* make backup before reduction */
		IntegerSet domBackup = copyIntegerSet(domainOfVar(var1));
	  varPos seqBackup = *(sequencePosition(var1));	
	  
	  /* check if domain of variable can be reduced by arc */
	  int reduced = arcReduce(arc, p);
	  freeArc(arc);
	  
	  /* if domain of variable is reduced by arc */
		if(reduced) {			
		  /* add backup of old domain and sequence position */
		  addBackup(var1->index, domBackup, seqBackup, backup);
		  /* resort variable in sequence */
			resortVarSeq(p->varSequence, sequencePosition(var1));
			addLog(
			  "X%d = %d -> Domain limited of variable X%d.\n", 
		    indexOfVar(var), domainMinimumOfVar(var), indexOfVar(var1)
		  );				
			int domSize = domainSizeOfVar(var1);
			/* if domain of variable became empty after reduction */
		  if(domSize == 0) {
		    makeArcQueueEmpty(arcQueue);
		    freeQueue(arcQueue);
		    /* return error */
			  return 0;
			} 
		} else {
		  /* backup not needed */
		  freeIntegerSet(domBackup);
		}
	}	
	freeQueue(arcQueue);
	
	/* queue is empty and no error occured */
	return 1;
}

/* 
  Function that performs constraint propagation after assignment
  If CP is set to MAC, then arc-consistency is maintained.
*/
int mac(Variable var, Problem p, Backup backup) {  
  Queue arcQueue = emptyQueue();
	
	addLog("Maintaining arc consistency after assignment of variable X%d\n", var->index);
	
	/* enqueue all arcs directed at var, so all (X --> var) */
  addVariableArcs(var, arcQueue, p);
  
  /* enqueue all new arcs (two directions of constraints that had arity 3
      before assignment of var, but became binary after) */
  addNewArcs(var, arcQueue, p);
  
  /* for all arcs in queue */
  while(!isEmptyQueue(arcQueue)) {		
    /* dequeue arc */					
    DirectedArc arc = dequeue(arcQueue);
    Constraint c = constraintOfArc(arc);
    Variable var1 = firstVarOfArc(arc);
    
    /* set arc 'not available' */
    inArcsQueue[indexOfConstraint(c)][indexOfVar(var1)] = 0;		
		
		/* make backup before reduction */
		IntegerSet domBackup = copyIntegerSet(domainOfVar(var1));
	  varPos seqBackup = *(sequencePosition(var1));	
	  
	  /* check if domain of variable can be reduced by arc */
	  int reduced = arcReduce(arc, p);
	  freeArc(arc);
	  
	  /* if domain of variable is reduced by arc */
		if(reduced) {			
		  /* add backup of old domain and sequence position */
		  addBackup(var1->index, domBackup, seqBackup, backup);
		  /* resort variable in sequence */
			resortVarSeq(p->varSequence, sequencePosition(var1));
			addLog(
			  "X%d = %d -> Domain limited of variable X%d.\n", 
		    indexOfVar(var), domainMinimumOfVar(var), indexOfVar(var1)
		  );				
			int domSize = domainSizeOfVar(var1);
			/* if domain of variable became empty after reduction */
		  if(domSize == 0) {
		    makeArcQueueEmpty(arcQueue);
		    freeQueue(arcQueue);
		    /* return error */
			  return 0;
			} 
			/* apply constraint propagation for variable with reduced domain */
			addVariableArcs(var1, arcQueue, p);	
		} else {
		  /* backup not needed */
		  freeIntegerSet(domBackup);
		}
	}	
	freeQueue(arcQueue);
	
	/* queue is empty and no error occured */
	return 1;
}

Variable selectUnassignedVar(Problem p) {
	VarPos position = firstVarPosition(p->varSequence);
	Variable v = varAtPosition(p->varSequence, position);
	return v;
}



int init(Problem p) {
	int i, j;
	p->assignCount = 0;
	
	int possibilities = 0;
	int maxDomSize = 0;
	int *domainCounts = safeCalloc(1, sizeof(int));
	for(i = 0; i < p->varCount; i++) {
		int domSize = domainSizeOfVar(varByIndex(p, i));
		possibilities += domSize;
		if(domSize > maxDomSize) {
			domainCounts = safeRealloc(domainCounts, (domSize+1)*sizeof(int));
			for(j = maxDomSize+1; j <= domSize; j++) {
				domainCounts[j] = 0;
			}
			maxDomSize = domSize;
		}
		domainCounts[domSize]++;
	}
	addLog("\n");
	addLog(
	  "# Before init: %d possible values and %d ", 
	  possibilities, (domainCounts[0] == 0)
  );
	for(i = 1; i <= maxDomSize; i++) {
		if(domainCounts[i]) {
			addLog("* %d^%d ", i, domainCounts[i]);
		}
	}
	addLog("combinations\n");
	addLog(
	  "# Applying substitution for single-value domains \
	  and checking applicable constraints:\n"
	);
	
	switch(MAKECONSISTENT) {
		case NODE:
			makeNodeConsistent(p);
			break;
		case ARC:
			makeArcConsistent(p);
			break;
	}
	
	switch(CP) {
	  case FC:
	    propagationSuccess = forwardChecking;
	    break;
	  case MAC:
	    propagationSuccess = mac;
	    break;
	  default:
	    propagationSuccess = skipPropagationTest;
	    break;
	}
	
	possibilities = 0;
	for(i = 0; i <= maxDomSize; i++) {
		domainCounts[i] = 0;
	}
	for(i = 0; i < p->varCount; i++) {
		int domSize = domainSizeOfVar(varByIndex(p, i));
		possibilities += domSize;
		domainCounts[domSize]++;
	}
	addLog(
	  "# After init: %d possible values and %d ", 
	  possibilities, (domainCounts[0] == 0)
	);
	for(i = 1; i <= maxDomSize; i++) {
		if(domainCounts[i]) {
			addLog("* %d^%d ", i, domainCounts[i]);
		}
	}
	addLog("combinations\n");
	free(domainCounts);
	addLog("# After init: domains\n");
	printVarDomains(p);
	initdone = 1;
	return 1;
}


SolutionSet newSolutionSet(int varAmount, int solutionSpace) {
	SolutionSet set = safeMalloc(sizeof(solutionSet));
	set->first = NULL;
	set->last = NULL;
	set->solutionSpace = solutionSpace;
	set->solutionCount = 0;
	set->varAmount = varAmount;
	return set;
}

 void printSolution(int *solution, int varCount) {
	int i;
	
	for(i = 0; i < varCount; i++) {
		printf("%d ", solution[i]);
	}
}

SolutionList newSolution(Problem p) {
	int i;
	SolutionList list = safeMalloc(sizeof(solutionList));
	list->values = safeMalloc(p->varCount*sizeof(int));
	for(i = 0; i < p->varCount; i++) {
		list->values[i] = domainMinimumOfVar(varByIndex(p, i));
	}
	list->next = NULL;
	return list;
}

void addSolution(SolutionSet solset, Problem p) {
	if(solset->solutionCount == solset->solutionSpace) {
		fprintf(stderr, "No more solutions can be added to this set\n");
		exit(-1);
	}
	if(solset->solutionCount == 0) {
		solset->first = newSolution(p);
		solset->last = solset->first;
	} else {
		solset->last->next = newSolution(p);
		solset->last = solset->last->next;
	}
	solset->solutionCount++;
}

int solutionsLeft(SolutionSet solset) {
  return (solset->solutionSpace - solset->solutionCount != 0);
}

void freeSolutionList(SolutionList list) {
	if(list != NULL) {
		freeSolutionList(list->next);
		free(list->values);
		free(list);
	}
}

void freeSolutionSet(SolutionSet set) {
	freeSolutionList(set->first);
	free(set);
}

void addVarToConstraints(Variable var, Problem p) {
  int i;
  int *constraintIndices = constraintIndicesOfVar(var);
  
  /* for every constraint c of var */
  for(i = 0; i < constraintAmountOfVar(var); i++) {
    Constraint c = constraintByIndex(p, constraintIndices[i]);
    
    /* if arity of constraint is unary, becomes binary */
    if(arityOfConstraint(c) == 1) {
      int *varIndices = varIndicesOfConstraint(c);
      Variable var2 = varByIndex(p, varIndices[0]);
      /* other variable gets constraint on other variable (var) */
      plusDegree(var2);
      /* var2 is still in sequence -> resort */
      resortVarSeq(p->varSequence, sequencePosition(var2));
    }
    
    int j;
    for(j = 0; j < arityOfConstraint(c); j++) {
      int *varIndices = varIndicesOfConstraint(c);
      Variable var2 = varByIndex(p, varIndices[j]);
      lowerConnectivity(var2);
    }
    
    /* add var to c */
    addVarToConstraint(c, var);
  }
}

void resetVar(Problem p, Variable v, varPos pos, IntegerSet dom) {
	setDomainOfVar(v, dom);
	addVarToConstraints(v, p);
	restoreVarSeq(p->varSequence, pos);
}

void removeVarFromConstraints(Variable var, Problem p) {
  int i;
  int *constraintIndices = constraintIndicesOfVar(var);
  
  /* for every constraint c of var */
  for(i = 0; i < constraintAmountOfVar(var); i++) {
    Constraint c = constraintByIndex(p, constraintIndices[i]);
    /* remove var from c */
    removeVarFromConstraint(c, var);
    
    /* if arity of constraint was binary, became unary */
    if(arityOfConstraint(c) == 1) {
      int *varIndices = varIndicesOfConstraint(c);
      Variable var2 = varByIndex(p, varIndices[0]);
      /* other variable loses constraint on other variable (var) */
      lowerDegree(var2);
      /* var2 is still in sequence -> resort */
      resortVarSeq(p->varSequence, sequencePosition(var2));
    }  
    
    int j;
    for(j = 0; j < arityOfConstraint(c); j++) {
      int *varIndices = varIndicesOfConstraint(c);
      Variable var2 = varByIndex(p, varIndices[j]);
      plusConnectivity(var2);
    }  
  }
}

void recursiveBacktracking(Problem p, SolutionSet solset) {		
	Variable var;
	varPos sequencePos;
	int *values, i;
	IntegerSet fullDomain;
	
	stateCount++;
	if(p->varCount == p->assignCount) {
		addSolution(solset, p);
		return;
	}
	
	var = selectUnassignedVar(p);
		
	sequencePos = *sequencePosition(var);
	fullDomain = domainOfVar(var);
	values = valuesOfSet(fullDomain);
	removeVarFromConstraints(var, p);
	removeVarFromSequence(p->varSequence, var->sequencePos);
	
	for(i = 0; i < sizeOfSet(fullDomain) && solutionsLeft(solset); i++) {
		addLog("Trying value %d for variable X%d.\n", values[i], var->index);
		p->assignCount++;
		setDomainOfVar(var, createSingletonDomain(values[i]));
		Backup backup = emptyBackup();	
		if(isConsistent(var, p, backup)) {
		  recursiveBacktracking(p, solset);	
		}
		restoreBackup(backup, p);
		freeIntegerSet(domainOfVar(var));
		p->assignCount--;
	}
	resetVar(p, var, sequencePos, fullDomain);
}

int checkConstantConstraints(Problem p) {
	int i;
	for(i = 0; i < p->constraintCount; i++) {
		if(arityOfConstraint(p->constraints[i]) == 0 && 
		   !satisfiable(p->constraints[i], p)) {
			addLog("constant constraint %d is not satisfiable:\n", i);
			printConstraint(p->constraints[i]);
			return 0;
		}
	}
	return 1;
}


SolutionSet solve(Problem p) {
	int i;
	
	/* set right function pointers */
	if(MRV) {
		varOrdered = mrvOrdered;
		if(DEGREE_HEURISTIC) {
			varOrdered = mrvPlusDegreeOrdered;
		} else if(MOSTCONNECTED) {
		  varOrdered = mrvPlusConnectedOrdered;
		}
	} else if(MOSTCONNECTED) {
	  varOrdered = mostConnectedOrdered;
	} else {
		varOrdered = skipTest;
	}
	
	inArcsQueue = safeMalloc(p->constraintCount * sizeof(int *));
	for(i = 0; i < p->constraintCount; i++) {
		inArcsQueue[i] = safeCalloc(p->varCount, sizeof(int));
	}
	
	SolutionSet solset = newSolutionSet(p->varCount, 
			(p->solvespec.type == SOLVEALL ? -1 : p->solvespec.max));
	
	p->varSequence = emptyVarSeq();
	
	addLog("\n####################################################\n");	
	
	if(checkConstantConstraints(p)) {	
	  /* for all constraints of problem */
	  for(i = 0; i < p->constraintCount; i++) {
	    Constraint c = constraintByIndex(p, i);
	    /* if arity of c > 1 */
	    if(arityOfConstraint(c) > 1) {      
	      int *varIndices = varIndicesOfConstraint(c);
	      int j;
	      /* for all variables of c */
	      for(j = 0; j < arityOfConstraint(c); j++) {
	        /* degree++ */
	        plusDegree(varByIndex(p, varIndices[j]));
	        
	        int k;
	        for(k = j+1; k < arityOfConstraint(c); k++) {
	          plusVarConnections(varByIndex(p, varIndices[j]));
	          plusVarConnections(varByIndex(p, varIndices[k]));
	        }
	      }
	    }
	  }	  
	  
	  init(p);
		int i;
		for(i = 0; i < p->varCount; i++) {
			insertVarInSequence(p->varSequence, varByIndex(p, i));
		}
		recursiveBacktracking(p, solset);
	}
	
	for(i = 0; i < p->constraintCount; i++) {
		free(inArcsQueue[i]);
	}
	free(inArcsQueue);
	
	fprintf(logFile, "backtracking points: %ld\n", stateCount);
	
	printf("%ld\n", stateCount);
					 
	return solset;
}
