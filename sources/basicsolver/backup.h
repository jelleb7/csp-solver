#ifndef BACKUP_H
#define BACKUP_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "variable.h"
#include "problem.h"

typedef struct backup *Backup;
typedef struct backupList *BackupList;

typedef struct backupList {
	int index;
	IntegerSet domain;
	varPos sequencePos;
	BackupList next;
} backupList;

typedef struct backup {
	BackupList first;
	BackupList last;
} backup;

Backup emptyBackup();
void freeBackup(Backup db);
void restoreBackup(Backup db, Problem p);
void addBackup(int varIndex, IntegerSet domain, varPos sequencePos, Backup db);

void printBackup(Backup b);


#endif
