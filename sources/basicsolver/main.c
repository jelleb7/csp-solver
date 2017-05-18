#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "datatypes.h"
#include "solve.h"
#include <limits.h>

FILE *logFile;

extern void parser(Problem LLuserData, Problem *LLretval);

void Main() {
  	Problem p = NULL;
  
  	parser(p, &p);
  
  	addLog("Accepted\n"); 	
  		
  	SolutionSet solset = solve(p);
  	SolutionList solution = solset->first;
	
	printf("%d \n", solset->solutionCount);
	
	  int i = 0;
	  while(solution != NULL) {
		  //printf("\n########## SOLUTION #%d ##########\n", i+1);
		  printSolution(solution->values, p->varCount);
		  solution = solution->next;
		  i++;
	  }
		
		/*
	  if(solset->solutionCount == 0) {
		  printf("\nNo solution could be found\n");
	  } else if(p->solvespec.type == SOLVEALL) {
		  printf("\nFound %d solution(s).\n", solset->solutionCount);
	  } else if(solset->solutionCount < p->solvespec.max) { 
		 
		  printf("\nNo more than %d solution(s) "
		  "could be found.\n", solset->solutionCount);
	  }
	  */
	
	  freeSolutionSet(solset);
  	
  	freeProblem(p);
}

void timedMain() {
  	clock_t start, stop;
  	start = clock();
  	Main();
  	stop = clock();
  	printf ("Cpu time: %.3f seconds\n", ((double)stop - (double)start)*1.0e-6);
}

int main(int argc, char *argv[]) { 	
	FILE *input;

	if(argc < 2) {
		fprintf(stderr, "[ERROR] No filename given\n");
		exit(-1);
	}

  	enableHeuristics(argc-2, &(argv[1]));

	input = freopen(argv[argc-1], "r", stdin);
	if(input == NULL) {
		fprintf(stderr, "[ERROR] Could not open file %s\n", argv[argc-1]);
		exit(-1);
	}

	stdout = fopen("solution.txt", "w");
	if(stdout == NULL) {
		fprintf(stderr, "[ERROR] Could not open file solution.txt\n");
		exit(-1);
	}

  	logFile = fopen("csp.log", "w");

	if (logFile == NULL) {
  	 	fprintf(stderr, "[ERROR] Error opening log file!\n");
   		exit(-1);
	}


  	//timedMain();	
	
	Main();
	
	fclose(logFile);
	fclose(stdout);
	fclose(input);
	  
  	return EXIT_SUCCESS;
}
