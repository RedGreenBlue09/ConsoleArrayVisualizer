
#include "Utils/LinkedList.h"

#include <assert.h>

// TODO: inline

void LinkedList_InitializeNode(llist_node* pNode) {
	assert(pNode);
	pNode->pPreviousNode = NULL;
	pNode->pNextNode = NULL;
};

void LinkedList_Insert(llist_node* pNode, llist_node* pNewNode) {
	assert(pNode);
	assert(pNewNode);
	pNewNode->pPreviousNode = pNode;
	pNewNode->pNextNode = pNode->pNextNode;
	pNode->pNextNode = pNewNode;
}

void LinkedList_Remove(llist_node* pNode) {
	assert(pNode);
	if (pNode->pPreviousNode != NULL)
		pNode->pPreviousNode->pNextNode = pNode->pNextNode;
	if (pNode->pNextNode != NULL)
		pNode->pNextNode->pPreviousNode = pNode->pPreviousNode;
}
