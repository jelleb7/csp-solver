#include <stdio.h>
#include <stdlib.h>
#include "types.h"

void *safeMalloc(size_t size) {
	void *retval = malloc(size);
	assert(retval != NULL);
	return retval;
}

void *safeCalloc(int amount, size_t size) {
	void *retval = calloc(amount, size);
	assert(retval != NULL);
	return retval;
}

Substitution newSubstitution(char *varname, int value) {
	Substitution s = safeMalloc(sizeof(substitution));
	s->var = varname;
	s->val = value;
	return s;
}




List getZeroIndices(int length) {
	if(length == 0) {
		return NULL;
	}
	return newList(0, getZeroIndices(length-1));
}

RecursiveType newRecursiveType(Type type, void *data) {
	RecursiveType rt = safeMalloc(sizeof(recursiveType));
	*rt = (recursiveType){type, data};
	return rt;
}

ForAll newForAll(char *varname, List subdoms, List items) {
	ForAll fa = safeMalloc(sizeof(forall));
	*fa = (forall){varname, subdoms, items};
	return fa;
}

DomainSet newDomainSet(List varList, List doms) {
	DomainSet ds = safeMalloc(sizeof(domainset));
	*ds = (domainset){varList, doms};
	return ds;
}

VarCall newVarCall(char *name, List indices) {
	VarCall vc = safeMalloc(sizeof(varcall));
	*vc = (varcall){name, indices};
	return vc;
}

Subdomain newSubdomain(NumExp min, NumExp max) {
	Subdomain d = safeMalloc(sizeof(subdomain));
	*d = (subdomain){min, max};
	return d;
}


Constraint newConstraint(NumExp exp1, char *op, NumExp exp2) {
	Constraint c = safeMalloc(sizeof(constraint));
	c->exp1 = exp1;
	c->operator = op;
	c->exp2 = exp2;
	return c;
}

Value newValue(Type vtype, void *data, int powtok, Factor exponent) {
	Value v = safeMalloc(sizeof(value));
	v->type = vtype;
	v->data = data;
	v->powtok = powtok;
	v->exponent = exponent;
	return v;
}

NumExp newNumExp(List termList, List opList) {
	NumExp ne = safeMalloc(sizeof(numExp));
	ne->termList = termList;
	ne->operatorList = opList;
	return ne;
}

Term newTerm(List factorList, List opList) {
	Term t = safeMalloc(sizeof(term));
	t->factorList = factorList;
	t->operatorList = opList;
	return t;
}

Factor newFactor(Type ftype, void *data) {
	Factor f = safeMalloc(sizeof(factor));
	f->type = ftype;
	f->data = data;
	return f;
}

FunctionCall newFunctionCall(Type type, char *funcName, List argList) {
	FunctionCall fc = safeMalloc(sizeof(functionCall));
	fc->type = type;
	fc->funcName = funcName;
	fc->argList = argList;
	return fc;
}

void *array(int size) {
	void *data = safeMalloc(size*sizeof(void *));
	return data;
}

