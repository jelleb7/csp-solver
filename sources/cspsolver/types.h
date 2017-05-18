#ifndef TYPES_H
#define TYPES_H

#include "list.h"
#include "vardb.h"

typedef struct vardef *VarDef;
typedef struct varcall *VarCall;

typedef struct constraint *Constraint;
typedef struct numExp *NumExp;
typedef struct term *Term;
typedef struct factor *Factor;
typedef struct value *Value;
typedef struct functionCall *FunctionCall;

typedef struct recursiveType *RecursiveType;
typedef struct domainset *DomainSet;
typedef struct forall *ForAll;
typedef struct substitution *Substitution;
typedef struct subdomain *Subdomain;

typedef enum {
	FUNCTIONCALL, INT,
	VALUE, FACTOR, NUMEXP, CONSTRAINT, FORALL, DOMSET, NEGATION, VARSUBSTCALL,
	MAXMIN, ALLANY, ABS, ALLDIFF, SUM, PRODUCT, MAXIMUM, MINIMUM, EQUAL, INCREASING, DECREASING
} Type;

typedef struct vardef {
	char *name;
	List dimsizes;
} vardef;

typedef struct varcall {
	char *name;
	List indices;
} varcall;

typedef struct substitution {
	char *var;
	int val;
} substitution;

typedef struct subdomain {
	NumExp min;
	NumExp max;
} subdomain;

typedef struct domainset {
	List varlist;
	List doms;
} domainset;

typedef struct recursiveType {
	Type type;
	void *data;
} recursiveType;	

typedef struct forall {
	char *varname;
	List subdomains;
	List items;
} forall;



typedef struct numExp {
	List termList;
	List operatorList;
} numExp;

typedef struct constraint {
	NumExp exp1;
	char *operator;
	NumExp exp2;
} constraint;

typedef struct term {
	List factorList;
	List operatorList;
} term;

typedef struct factor {
	Type type;
	void *data;
} factor;

typedef struct value {
	Type type;
	void *data;
	int powtok;
	Factor exponent;
} value;

typedef struct functionCall {
	Type type;
	char *funcName;
	List argList;
} functionCall;

void *safeMalloc(size_t size);
void *safeCalloc(int amount, size_t size);

int getGlobalIndex(Variable var, int subidx);
Substitution newSubstitution(char *varname, int value);
int calcSubIndex(int dims, List dimsizes, List indices, List substset);
List getZeroIndices(int length);
vardef newVarDef();
VarCall newVarCall(char *name, List indices);
RecursiveType newRecursiveType(Type type, void *data);
ForAll newForAll(char *varname, List subdoms, List items);
DomainSet newDomainSet(List varList, List doms);
Subdomain newSubdomain(NumExp min, NumExp max);

Constraint newConstraint(NumExp exp1, char *op, NumExp exp2);
Value newValue(Type vtype, void *data, int powtok, Factor powFactor);
NumExp newNumExp(List explist, List opList);
Term newTerm(List factorList, List opList);
Factor newFactor(Type ftype, void *data);
FunctionCall newFunctionCall(Type type, char *funcName, List argList);

#endif
