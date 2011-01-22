/*
 *  FMList.h
 *  FountainMusic
 *
 *  Copyright 2003-2011 Brian Moore
 *
 */

#ifndef _FMList_H
#define _FMList_H

#ifdef FM_WINDOWS_BUILD
#include "iTunesAPI.h"
#else

#endif

typedef struct _FMListNode FMListNode; 

struct _FMListNode
{
    FMListNode *prev;
    void *data;
    FMListNode *next;
};

FMListNode *FMNewListNode(void *newData);
void FMDeleteListNode(FMListNode *killNode);

void FMListInsertNodeAfter(FMListNode *node, FMListNode *nodeAfter);
void FMListInsertNodeBefore(FMListNode *node, FMListNode *nodeBefore);

// precondition: node is not the next node in an enumerator
void FMNodeDetach(FMListNode *node);

typedef struct _FMList
{
    FMListNode *head;
    FMListNode *tail;
    // int count;
} FMList;

FMList *FMNewList(); // null everything
void FMDeleteList(FMList *list); // releases nodes

FMListNode *FMListDetachHead(FMList *list);
FMListNode *FMListDetachTail(FMList *list);
void FMListDetachNode(FMList *list, FMListNode *node);

void FMListAppendNode(FMList *list, FMListNode *node);
void FMListPrependNode(FMList *list, FMListNode *node);

void FMListAppendData(FMList *list, void *dataPtr);
void FMListPrependData(FMList *list, void *dataPtr);

typedef struct _FMListEnumerator
{
	FMList *list;
    FMListNode *currNode;
	FMListNode *nextNode;
} FMListEnumerator;

FMListEnumerator *FMNewListEnumerator(FMList *list);
void FMDeleteListEnumerator(FMListEnumerator *enumerator);

FMListNode *FMListEnumeratorNext(FMListEnumerator *enumerator);
void *FMListEnumeratorNextData(FMListEnumerator *enumerator);
FMListNode *FMListEnumeratorCurrent(FMListEnumerator *enumerator);
Boolean FMListEnumeratorIsDone(FMListEnumerator *enumerator);

FMListNode *FMListEnumeratorRemoveCurrent(FMListEnumerator *enumerator);

#endif

