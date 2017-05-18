#include "rewrite.h"

void printNFVar(int idx) {
	printf("X%d", idx);
}

void printComma(int boolean) {
	if(boolean) {
		printf(", ");
	}
}


void rewriteVarDef(VarDef vd, int comma) {
	printComma(comma);
	Variable var = getVariableFromDB(vd->name);
	int dims = var->dims;
	
	if(dims == 0) {
		int idx = var->globalIndex;
		printNFVar(idx);
	} else { /* dims > 0 */
		int i;
		int count = 1;
		List dimsizes = vd->dimsizes;
		while(dimsizes != NULL) {
			count *= *((int *) dimsizes->item);
			dimsizes = dimsizes->next;
		}
		for(i = 0; i < count; i++) {
			printComma(i);
			int idx = getGlobalIndex(var, i);
			printNFVar(idx);
		}
		
	}
}

void rewriteVarDefList(List vl) {
	rewriteVarDef(vl->item, 1);
	vl = vl->next;
	while(vl != NULL) {
		VarDef vd = vl->item;
		rewriteVarDef(vd, 0);
		vl = vl->next;
	}
}

void rewriteVarCall(VarCall vc, List substset) {
	char *varname = vc->name;
	Variable var = getVariableFromDB(varname);
	int dims = var->dims;
	List dimsizes = var->dimsizes;
	List indices = vc->indices;
	int subidx = calcSubIndex(dims, dimsizes, indices);
	int idx = getGlobalIndex(var, subidx);
	printNFVar(idx);
}

void rewriteVarList(List vl, List substset) {
	rewriteVarCall(vl->item, substset);
	vl = vl->next;
	while(vl != NULL) {
		printComma(1);
		rewriteVarCall(vl->item, substset);
		vl = vl->next;
	}
}

void rewriteDomain(domain d) {
	printf("[%d..%d]", d.min, d.max);
}

void rewriteDomainSpec(RecursiveType domspec, List substset) {
	if(domspec->type == DOMSET) {
		DomainSet domset = (DomainSet) domspec->data;
		rewriteVarList(domset->varlist, substset);
		printf(" <- ");
		rewriteDomain(domset->dom);
		printf(";\n");
	} else { /* domainspec->type == FORALL */
		ForAll forall = (ForAll) domspec->data;
		char varname = forall->varname;
		List values = forall->values;
		while(values != NULL) {
			substitution subst = newSubstitution(varname, *((int*)values->item));
			List newset = newList(&subst, substset);
			List domspeclist = forall->items;
			while(domspeclist != NULL) {
				rewriteDomainSpec(domspeclist->item, newset);
				domspeclist = domspeclist->next;
			}
			free(newset->item);
			free(newset);
			values = values->next;
		}
	}
}

void rewriteAllDiff(VarCall vc) {
	Variable var = getVariableFromDB(vc->name);
	int vardims = var->dims;
	List indices = vc->indices;
	int dimdepth = listLength(indices);
	List last = indices;
	while(last->next != NULL) {
		last = last->next;
	}
	last->next = getZeroIndices(vardims-dimdepth);
	int firstidx = calcSubIndex(vardims, var->dimsizes, indices);
	last->item = last->item + 1;
	int lastidx = calcSubIndex(vardims, var->dimsizes, indices);
	int absidx = getGlobalIndex(var, firstidx);
	int i,j;
	for(i = firstidx; i < lastidx; i++) {
		for(j = i+1; j < lastidx; j++) {
			printNFVar(absidx+i);
			printf(" <> ");
			printNFVar(absidx+j);
			printf(";\n");
		}
	}
}

void rewriteConstraintSpec(RecursiveType cs, List substset) {
	if(cs->type == CONSTRAINT) {
		
	} else { /* cs->type == FORALL */
		ForAll forall = cs->data;
		char varname = forall->varname;
		List values = forall->values;
		while(values != NULL) {
			substitution subst = newSubstitution(varname, *((int *)values->item));
			List newset = newList(&subst, substset);
			List cslist = forall->items;
			while(cslist != NULL) {
				rewriteConstraintSpec(cslist->item, newset);
				cslist = cslist->next;
			}
			free(newset->item);
			free(newset);
			values = values->next;
		}
	}
}
