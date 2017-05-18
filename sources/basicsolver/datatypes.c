#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "datatypes.h"
#include "problem.h"
#include "variable.h"
#include "constraint.h"
#include "solve.h"
#include "string.h"

void *safeMalloc(size_t size) {
	void *ret = malloc(size);
	assert(ret != NULL);
	return ret;
}

void *safeCalloc(int amount, size_t size) {
	void *ret = calloc(amount, size);
	assert(ret != NULL);
	return ret;
}

void *safeRealloc(void *oldPointer, size_t size) {
	void *ret = realloc(oldPointer, size);
	assert(ret != NULL);
	return ret;
}


/* BEGIN functions regarding datatype IntegerList */


IntegerList newIntegerList(int val, IntegerList next) {
	IntegerList intList = safeMalloc(sizeof(integerList));
	intList->val = val;
	intList->next = next;
	if(next != NULL) {
		next->prev = intList;
	}
	return intList;
}

IntegerList copyIntegerList(IntegerList orig) {
	if(orig == NULL) {
		return NULL;
	}
	IntegerList copy = safeMalloc(sizeof(integerList));
	copy->val = orig->val;
	copy->prev = NULL;
	copy->next = copyIntegerList(orig->next);
	copy->next->prev = copy;
	return copy;
}

void printIntegerList(IntegerList list) {
	while(list != NULL) {
		printf("%d ", list->val);
		list = list->next;
	}
	printf("\n");
}

void freeIntegerList(IntegerList list) {
	if(list != NULL) {
		freeIntegerList(list->next);
		free(list);
	}
}

/* END functions regarding datatype IntegerList */


/* BEGIN functions regarding datatype IntegerSet */

IntegerSet emptyIntegerSet() {
	IntegerSet set = safeMalloc(sizeof(integerSet));
	set->size = 0;
	set->space = 8;
	set->values = safeMalloc(set->space*sizeof(int));
	return set;
}

void doubleSetSpace(IntegerSet set) {
	set->space *= 2;
	set->values = safeRealloc(set->values, set->space*sizeof(int));
}

IntegerSet copyIntegerSet(IntegerSet orig) {
	IntegerSet copy = safeMalloc(sizeof(integerSet));
	copy->size = orig->size;
	copy->space = orig->space;
	copy->values = safeMalloc(copy->space*sizeof(int));
	copy->values = memcpy(copy->values, orig->values, copy->size*sizeof(int));
	return copy;
}

void freeIntegerSet(IntegerSet set) {
	free(set->values);
	free(set);
}

void printIntegerSet(IntegerSet set) {
	int i;
	printf("[");
	if(set->size > 0) {
		printf("%d", set->values[0]);
		for(i = 1; i < set->size; i++) {
			printf(", %d", set->values[i]);
		}
	}
	printf("]");
}

void checkSorted(IntegerSet set) {
	int i;
	for(i = 1; i < set->size; i++) {
		if(set->values[i] < set->values[i-1]) {
			printf("not sorted: ");
			printIntegerSet(set);
			printf("\n");
			exit(-1);
		}
	}
}

int findIndex(int *array, int size, int value) {
	int begin = 0;
	int end = size-1;	
	
	while(begin <= end) {
		int mid = (begin + end) / 2;
		if(array[mid] > value) {
			end = mid-1;
		} else if(array[mid] < value) {
			begin = mid+1;
		} else {	/* array[mid] == value */
			return mid;
		}
	}
	/* begin > end -> value not found */
	return -(begin+1);
}

/*
 * Function that shifts values of array of IntegerSet set
 * The interval [start, start+len] is shifted to [dest, dest+len] 
*/ 
void shiftValues(IntegerSet set, int start, int len, int dest) {
	int i;
	while(dest+len >= set->space) {
		doubleSetSpace(set);
	}
	if(start < dest) {
		for(i = len-1; i >= 0; i--) {
			set->values[dest+i] = set->values[start+i];
		}
	} else if(start > dest) {	
		for(i = 0; i < len; i++) {
			set->values[dest+i] = set->values[start+i];
		}
	}	/* else -> start == dest -> nothing to do */
}

/*
static void printSet(IntegerSet set) {
  int i;
  for(i = 0; i < sizeOfSet(set); i++) {
    printf(" %d ", set->values[i]);
  }
  printf("\n");
}*/


void addIntegerToSet(IntegerSet set, int value) {
	int idx = findIndex(set->values, set->size, value);
	if(idx >= 0) {
		/* item is already available in set */
		return;
	}
	/* idx < 0 -> not in set, but should be inserted at position -(idx+1) */
	idx = -(idx+1);
	/* shift all values from index [idx, set->size-1] to [idx+1, set->size] */
	shiftValues(set, idx, set->size - idx, idx+1);
	/* put value at right index */
	set->values[idx] = value;
	set->size++;
}

void removeNthIntegerFromSet(IntegerSet set, int n) {
	int start, len, dest;
	if(n >= set->size) {
		/* index not available */
		return;
	}
	/* shift all values from indices [n+1, set->size-1] to [n, set->size-2] */
	start = n+1;
	dest = n;
	len = set->size-start;
	shiftValues(set, start, len, dest);
	set->size--;
}

void removeIntegerFromSet(IntegerSet set, int value) {
	int idx = findIndex(set->values, set->size, value);
	if(idx >= 0) {
		removeNthIntegerFromSet(set, idx);
	}
}

void addIntervalToSet(IntegerSet set, int min, int max) {
	int sizePartToShift;
	int i;
	int intervalSize = max-min+1;
	int intervalBegin = findIndex(set->values, set->size, min);
	int beginPartToShift = findIndex(set->values, set->size, max+1);
	int shiftDest;
	
	if(intervalBegin < 0) {
		intervalBegin = -(intervalBegin+1);
	}
	if(beginPartToShift < 0) {
		beginPartToShift = -(beginPartToShift+1);
	}
	
	sizePartToShift = set->size - beginPartToShift;
	set->size = intervalBegin + intervalSize + sizePartToShift;
	shiftDest = intervalBegin + intervalSize;
	
	/* shift values in set that occur after interval after sequence */
	shiftValues(set, beginPartToShift, sizePartToShift, shiftDest);
	
	/* add all values of interval to set */
	for(i = 0; i < intervalSize; i++) {
		set->values[intervalBegin+i] = min+i;
	}
	checkSorted(set);
}

IntegerSet intersect(IntegerSet set1, IntegerSet set2) {
  IntegerSet returnSet;
  if(set1->size > set2->size) {
    IntegerSet tmp = set1;
    set1 = set2;
    set2 = tmp;
  }
  returnSet = copyIntegerSet(set1); /* copy smallest IntegerSet */
  int i = 0;
  while(i < returnSet->size) {
    if(!valueInSet(set2, returnSet->values[i])) {
      removeNthIntegerFromSet(returnSet, i);  /* remove value at index i */
    } else {
      i++;                            /* i-th value should remain in set */
    }
  }
  return returnSet;
}

IntegerSet except(IntegerSet total, IntegerSet toBeRemoved) {
  IntegerSet returnSet = copyIntegerSet(total);
  int i;
  for(i = 0; i < toBeRemoved->size; i++) {
    removeIntegerFromSet(returnSet, toBeRemoved->values[i]);
  }
  return returnSet;
}

int minimumOfSet(IntegerSet set) {
	if(set->size == 0) {
		fprintf(stderr, "Set is empty -> no minimum value can be returned\n");
		exit(-1);
	}
	return set->values[0];
}

int maximumOfSet(IntegerSet set) {
	if(set->size == 0) {
		fprintf(stderr, "Domain is empty -> no minimum value can be returned\n");
	}
	return set->values[set->size-1];
}

int sizeOfSet(IntegerSet set) {
	return set->size;
}

int *valuesOfSet(IntegerSet set) {
	return set->values;
}

int valueInSet(IntegerSet set, int value) {
  int idx = findIndex(set->values, set->size, value);
	return (idx >= 0);
}

/* END functions regarding datatype IntegerSet */

/* BEGIN Functions List */

void freeList(List l) {
	while(l != NULL) {
		List tmp = l->next;
		free(l);
		l = tmp;
	}
}

List newListItem(void *data, List next) {
	List l = safeMalloc(sizeof(listItem));
	l->data = data;
	l->next = next;
	return l;
}

/* END Functions List */


/* BEGIN Functions Queue */

Queue emptyQueue() {
	Queue q = safeMalloc(sizeof(queue));
	q->first = NULL;
	q->last = NULL;
	return q;
}

int isEmptyQueue(Queue q) {
	return (q->first == NULL);
}

void freeQueue(Queue q) {
	freeList(q->first);
	free(q);
}

void enqueue(Queue q, void *item) {
	if(q->last != NULL) {
		q->last->next = newListItem(item, NULL);
		q->last = q->last->next;
	} else {	/* Queue is empty */
		q->first = newListItem(item, NULL);
		q->last = q->first;
	}
}

void *dequeue(Queue q) {
	if(q->first == NULL) {
		fprintf(stderr, "Error @dequeue: Queue is empty\n");
		exit(-1);
	}
	List first = q->first;
	void *ret = first->data;
	q->first = first->next;
	if(q->first == NULL) {
		q->last = NULL;
	}
	free(first);
	return ret;
}

/* END Functions Queue */


/* BEGIN Functions Stack */

Stack emptyStack() {
  Stack s = safeMalloc(sizeof(stack));
	s->first = NULL;
	return s;
}

int isEmptyStack(Stack s) {
  return (s->first == NULL);
}

void freeStack(Stack s) {
  freeList(s->first);
  free(s);
}

void push(Stack s, void *item) {
  s->first = newListItem(item, s->first);
}

void *pop(Stack s) {
  if(s->first == NULL) {
		fprintf(stderr, "Error @pop: Stack is empty\n");
		exit(-1);
	}
	void *ret = s->first->data;
	List toFree = s->first;
	s->first = s->first->next;
	free(toFree);
	return ret;
}

void *top(Stack s) {
  if(s->first == NULL) {
		fprintf(stderr, "Error @top: Stack is empty\n");
		exit(-1);
	}
	return s->first->data;
}


