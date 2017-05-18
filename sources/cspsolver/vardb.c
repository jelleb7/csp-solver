#include <stdio.h> 
#include <stdlib.h> 
#include "rewrite.h"

#include "vardb.h"

Variable newVariable(char *name, int dims, List dimcounts) {
	Variable var = malloc(sizeof(variable));
	assert(var != NULL);
	var->name = name;
	var->dims = dims;
	var->dimsizes = dimcounts;
	var->globalIndex = currentIndex;
	var->totalCount = 1;
	while(dimcounts != 0) {
		int mult = calcNumExp(dimcounts->item, NULL);
		if(mult <= 0) {
			fprintf(stderr, "[ERROR] dimension sizes can not be zero or negative. \n");
			exit(-1);
		}
		var->totalCount *= mult;
		dimcounts = dimcounts->next;
	}
	var->domainset = safeCalloc(var->totalCount, sizeof(int));
	currentIndex = currentIndex + var->totalCount;
	return var;
}

List getAllVariables() {
	return globalVarDB->first;
}

void createEmptyVarDB() {
	globalVarDB = malloc(sizeof(vardb));
	assert(globalVarDB != NULL);
	globalVarDB->first = NULL;
	globalVarDB->last = NULL;
}

void freeVariable(Variable v) {
	free(v->name);
	freeList(v->dimsizes);
	free(v->domainset);
}

void freeVarList(List l) {
	if(l == NULL) {
		return;
	}
	freeVarList(l->next);
	freeVariable(l->item);
	free(l);
}

void freeVarDB() {
	freeVarList(globalVarDB->first);
}

void addVarToDB(Variable var) {
	List l = newList(var, NULL);
	if(globalVarDB->last == NULL) {
		globalVarDB->first = l;
		globalVarDB->last = l;
	} else {
		globalVarDB->last->next = l;
		globalVarDB->last = globalVarDB->last->next;	
	}
}

Variable getVariableFromDB(char *name) {
	List l = globalVarDB->first;
	while(l != NULL) {
		Variable var = (Variable) l->item;
		if(strcmp(name, var->name) == 0) {
			return var;
		}
		l = l->next;
	}
	return NULL;
}

void checkIndexDomainSet(int dimCnt, int dim, int *dimsizes, int *indices, int *domainset, char *varname) {
	if(dimCnt == dim) {
		int baseidx = dimsizes[dimCnt];
		if(dimCnt == 0) {
			baseidx = 0;
		} else {
			for(int i = 0; i < dimCnt; i++) {
				baseidx *= indices[i];
			}
		}
		for(int i = 0; i < dimsizes[dimCnt]; i++) {
			if(domainset[baseidx+i] == 0) {
				fprintf(stderr, "[ERROR] No domain set for %s", varname);
				for(int j = 0; j < dimCnt; j++) {
					fprintf(stderr, "[%d]", indices[j]);
				}
				fprintf(stderr, "[%d].\n", i);
				exit(-1);
			}
		}
	} else {
		for(int i = 0; i < dimsizes[dim]; i++) {
			indices[dim] = i;
			checkIndexDomainSet(dimCnt, dim+1, dimsizes, indices, domainset, varname);
		}
	}
}

void domainVarSet(Variable var, int globalIndex) {
	var->domainset[globalIndex-var->globalIndex] = 1;
}

void checkVarDomainsSet(Variable var) {	
	if(var->dims == 0) {
		if(!var->domainset[0]) {
			fprintf(stderr, "[ERROR] No domain set for %s", var->name);
		}
	} else {
		int *domainset = var->domainset;
		List dimsizesList = var->dimsizes;
		int *dimsizes = safeMalloc(var->dims*sizeof(int));
		int *indices = safeMalloc(var->dims*sizeof(int));
	
		for(int i = 0 ; i < var->dims; i++) {
			dimsizes[i] = calcNumExp(dimsizesList->item, NULL);
			dimsizesList = dimsizesList->next;
		}
		
		checkIndexDomainSet(var->dims-1, 0, dimsizes, indices, domainset, var->name);
		free(dimsizes);
		free(indices);
	}
}

void checkDomainsSet() {
	
	List varList = globalVarDB->first;
	while(varList != NULL) {
		Variable var = varList->item;
		checkVarDomainsSet(var);
		varList = varList->next;
	}
}
