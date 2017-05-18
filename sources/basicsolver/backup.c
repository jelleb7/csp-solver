#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "backup.h"
#include "solve.h"

Backup emptyBackup() {
	Backup db = safeMalloc(sizeof(backup));
	db->first = NULL;
	db->last = NULL;
	return db;
}

void freeBackupList(BackupList dbl) {
	if(dbl != NULL) {
		freeBackupList(dbl->next);
		free(dbl);
	}
}

void freeBackup(Backup db) {
	freeBackupList(db->first);
	free(db);
}


void restoreBackup(Backup db, Problem p) {
	BackupList list = db->first;
	while(list != NULL) { /* foreach (variable, domain) tuple -> restore */
		Variable v = p->vars[list->index];
		IntegerSet new = domainOfVar(v);
		setDomainOfVar(v, list->domain);
		if(varInSequence(list->sequencePos.var, p->varSequence)) {
			restoreVarSeq(p->varSequence, list->sequencePos);
		}
		freeIntegerSet(new);
		
		list = list->next;
	}
	freeBackup(db);
}


void addBackup(int varIndex, IntegerSet domain, varPos seqPos, Backup backup) {
	BackupList new = safeMalloc(sizeof(backupList));
	new->index = varIndex;
	new->domain = domain;
	new->sequencePos = seqPos;
	new->next = backup->first;
	backup->first = new;
	if(backup->last == NULL) {
		backup->last = new;
	}
}

void printBackup(Backup b) {
  addLog("BEGIN BACKUP\n");
  BackupList list = b->first;
  while(list != NULL) {
    Variable v = list->sequencePos.var;
    int varIndex = indexOfVar(v);
    int prevIdx = -1;
    int nextIdx = -1;
    if(list->sequencePos.prev != NULL) {
      prevIdx = indexOfVar(list->sequencePos.prev->var);
    }
    if(list->sequencePos.next != NULL) {
      nextIdx = indexOfVar(list->sequencePos.next->var);
    } 
    addLog("Variable X%d, prev: %d, next: %d\n", varIndex, prevIdx, nextIdx);
    list = list->next;
  }
  addLog("END BACKUP\n");
}


