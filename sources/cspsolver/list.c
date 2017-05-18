#include "list.h"
#include <stdlib.h>
#include <assert.h>

List newList(void *item, List next) {
	List l = (List) malloc(sizeof(list));
	assert(l != NULL);
	l->item = item;
	l->next = next;
	return l;
}

void freeList(List l) {
	if(l == NULL) {
		return;
	}
	freeList(l->next);
	free(l->item);
	free(l);
}

int listLength(List l) {
	int length = 0;
	while(l != NULL) {
		length++;
		l = l->next;
	}
	return length;
}

List addToListEnd(void *data, List l) {
	if(l == NULL) {
		return newList(data, NULL);
	}
	List ptr = l;
	while(ptr->next != NULL) {
		ptr = ptr->next;
	}
	ptr->next = newList(data, NULL);
	return l;
}
