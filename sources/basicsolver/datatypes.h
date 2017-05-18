#ifndef DATATYPES_H
#define DATATYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct integerList *IntegerList;
typedef struct integerSet *IntegerSet;
typedef struct listItem *List;
typedef struct queue *Queue;
typedef struct stack *Stack;

typedef enum {
	INTEGER, BOOLEAN
} DataType;

typedef struct integerSet {
	int size;
	int space;
	int *values;
} integerSet;

typedef struct integerList {
	int val;
	IntegerList next, prev;
} integerList;

typedef struct Tuple {
	int min;
	int max;
} Tuple;

typedef struct listItem {
	void *data;
	List next;
} listItem;

typedef struct queue {
	List first;
	List last;
} queue;

typedef struct stack {
  List first;
} stack;

void *safeMalloc(size_t size);
void *safeCalloc(int amount, size_t size);
void *safeRealloc(void *oldPtr, size_t size);

IntegerList newIntegerList(int val, IntegerList next);
IntegerList copyIntegerList(IntegerList orig);
void printIntegerList(IntegerList list);
void freeIntegerList(IntegerList list);

IntegerSet emptyIntegerSet();
IntegerSet copyIntegerSet(IntegerSet orig);
void freeIntegerSet(IntegerSet set);
void addIntegerToSet(IntegerSet set, int value);
void addIntervalToSet(IntegerSet set, int min, int max);
void removeIntegerFromSet(IntegerSet set, int value);
void removeNthIntegerFromSet(IntegerSet set, int n);
IntegerSet intersect(IntegerSet set1, IntegerSet set2);
IntegerSet except(IntegerSet total, IntegerSet toBeRemoved);

int minimumOfSet(IntegerSet d);
int maximumOfSet(IntegerSet d);
int sizeOfSet(IntegerSet set);
int *valuesOfSet(IntegerSet set);
int valueInSet(IntegerSet set, int value);

List newListItem(void *data, List next);
void freeList(List l);

Queue emptyQueue();
int isEmptyQueue(Queue q);
void freeQueue(Queue q);
void enqueue(Queue q, void *item);
void *dequeue(Queue q);

Stack emptyStack();
int isEmptyStack(Stack s);
void freeStack(Stack s);
void push(Stack s, void *item);
void *pop(Stack s);
void *top(Stack s);

#endif








