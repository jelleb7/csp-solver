#ifndef LIST_H
#define LIST_H

#include <assert.h>

typedef struct list *List;

typedef struct list {
	void *item;
	List next;
} list;

List newList(void *item, List next);
void freeList(List l);
int listLength(List l);
List addToListEnd(void *data, List l);

#endif
