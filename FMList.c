/*
 *  FMList.c
 *  FountainMusic
 *
 *  Copyright 2003, 2004, 2005, 2006 Brian Moore
 *
 */

#include "FMList.h"
#include "FMDefs.h"

static FMListEnumerator *stockEnumerator = NULL;
static Boolean stockInUse = FALSE;

FMListNode *FMNewListNode(void *newData)
{
    FMListNode *newNode = calloc(1, sizeof(FMListNode));
    
    if (!newNode)
    {
        DEBUG_OUT("Fountain Music Error: newNode could not be allocated in FMNewListNode()!\n");
        return NULL;
    }
    
    newNode->data = newData;
    newNode->next = NULL;
    newNode->prev = NULL;

    return newNode;
}

void FMDeleteListNode(FMListNode *killNode)
{
    if (killNode)
    {
        FMNodeDetach(killNode);
        
        free(killNode);
    }
}

void FMListInsertNodeAfter(FMListNode *baseNode, FMListNode *node)
{
     if (node && baseNode)
     {
        FMNodeDetach(node);
     
        node->prev = baseNode;
        node->next = baseNode->next;
        
        if (baseNode->next) baseNode->next->prev = node;
        baseNode->next = node;
    }
}

void FMListInsertNodeBefore(FMListNode *baseNode, FMListNode *node)
{
     if (node && baseNode)
     {
        FMNodeDetach(node);
     
        node->prev = baseNode->prev;
        node->next = baseNode;
        
        if (baseNode->prev) baseNode->prev->next = node;
        baseNode->prev = node;
     }
}

void FMNodeDetach(FMListNode *node)
{
    if (node)
    {
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        
        node->prev = NULL;
        node->next = NULL;
    }
}

FMListNode *FMListDetachTail(FMList *list)
{
    FMListNode *oldTail;
    
    oldTail = list->tail;
    
    if (oldTail)  list->tail = oldTail->prev;
    
    FMNodeDetach(oldTail);
    
    return oldTail;
}

FMListNode *FMListDetachHead(FMList *list)
{
    FMListNode *oldHead;
    
    oldHead = list->head;
    
    if (oldHead) list->head = oldHead->next;
    
    FMNodeDetach(oldHead);
    
    return oldHead;
}

FMList *FMNewList()
{
    FMList *newDLL = calloc(1, sizeof(FMList));
    
    if (!newDLL)
    {
        DEBUG_OUT("Fountain Music Error: newDLL could not be allocated in FMNewList()!\n");
        return NULL;
    }
    
    newDLL->head = NULL;
    newDLL->tail = NULL;
    
    return newDLL;
}
void FMDeleteList(FMList *list)
{
    FMListNode *currNode;
    FMListNode *currNext;
    
    currNode = list->head;
    
    while (currNode)
    {
        currNext = currNode->next;
        
        FMDeleteListNode(currNode);
        
        currNode = currNext;
    }
    
    free(list);
}

void FMListAppendNode(FMList *list, FMListNode *newNode)
{
    if (newNode)
    {
        if (list->tail && list->head)
        {
            FMListInsertNodeAfter(list->tail, newNode);
            
            list->tail = newNode;
        }
        else
        {
            list->tail = list->head = newNode;
        }
    }
}

void FMListPrependNode(FMList *list, FMListNode *newNode)
{
    if (newNode)
    {
        if (list->head && list->tail)
        {
            FMListInsertNodeBefore(list->head, newNode);
            
            list->head = newNode;
        }
        else
        {
            list->tail = list->head = newNode;
        }
    }
}

void FMListAppendData(FMList *list, void *dataPtr)
{
    FMListAppendNode(list, FMNewListNode(dataPtr));
}

void FMListPrependData(FMList *list, void *dataPtr)
{
    FMListPrependNode(list, FMNewListNode(dataPtr));
}

void FMListDetachNode(FMList *list, FMListNode *node)
{
    if (node == list->head) FMListDetachHead(list);
	if (node == list->tail) FMListDetachTail(list);
	FMNodeDetach(node);
}

FMListEnumerator *FMNewListEnumerator(FMList *list)
{
    FMListEnumerator *newEnumerator;
	
	if (stockEnumerator == NULL)
	{
		stockEnumerator = calloc(1, sizeof(FMListEnumerator));
	}
	
	if (stockInUse == FALSE)
	{
		newEnumerator = stockEnumerator;
		stockInUse = TRUE;
	}
	else newEnumerator = calloc(1, sizeof(FMListEnumerator));
    
    if (!newEnumerator)
    {
        DEBUG_OUT("Fountain Music Error: newEnumerator could not be allocated in FMNewListEnumerator()!\n");
        return NULL;
    }
    
    newEnumerator->currNode = NULL;
	newEnumerator->nextNode = list->head;
	newEnumerator->list = list;
     
    return newEnumerator;
}

void FMDeleteListEnumerator(FMListEnumerator *enumerator)
{
	if (enumerator == stockEnumerator) stockInUse = FALSE;
    else free(enumerator);
}

FMListNode *FMListEnumeratorNext(FMListEnumerator *enumerator)
{
	enumerator->currNode = enumerator->nextNode;
	
	if (enumerator->currNode) enumerator->nextNode = enumerator->currNode->next;
	else enumerator->nextNode = NULL;
	
	return enumerator->currNode;
}

void *FMListEnumeratorNextData(FMListEnumerator *enumerator)
{
	FMListNode *next;
	
	next = FMListEnumeratorNext(enumerator);
	
	if (next == NULL) return NULL;
	else return next->data;
}

FMListNode *FMListEnumeratorCurrent(FMListEnumerator *enumerator)
{
    return enumerator->currNode;
}

Boolean FMListEnumeratorIsDone(FMListEnumerator *enumerator)
{
    return (!enumerator->currNode) && (!enumerator->nextNode);
}

FMListNode *FMListEnumeratorRemoveCurrent(FMListEnumerator *enumerator)
{
	FMListNode *node = enumerator->currNode;
	
	enumerator->currNode = NULL;
	
	FMListDetachNode(enumerator->list, node);
	
	return node;
}
