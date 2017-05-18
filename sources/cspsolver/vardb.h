#ifndef VARDB_H
#define VARDB_H

#include <assert.h>
#include <string.h>
#include "list.h"

typedef struct vardb *VarDB;
typedef struct variable *Variable;

/*
typedef struct indexer *Index;
*/

int currentIndex;
VarDB globalVarDB;

typedef struct vardb {
	List first;
	List last;
} vardb;

typedef struct variable {
	char *name;
	int dims;
	List dimsizes;
	int *domainset;
	int globalIndex;
	int totalCount;
} variable;

/*
typedef struct indexer {
	indexType type;
	char *var;
	int val;
} indexer;
*/


List getAllVariables();
Variable getVariableFromDB(char *name);
Variable newVariable(char *name, int dims, List dimcounts);
void createEmptyVarDB();
void addVarToDB(Variable var);
void checkDomainsSet();
void domainVarSet(Variable var, int globalIndex);

#endif
