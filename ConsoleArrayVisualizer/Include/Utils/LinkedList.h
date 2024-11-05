#pragma once

#include <assert.h>
#include <stdint.h>

typedef struct llist_node_tag {
	struct llist_node_tag* pPreviousNode; // Slow indeed, but this is to support resizable pool
	struct llist_node_tag* pNextNode;
} llist_node;

// Create an alone node
static inline void LinkedList_InitializeNode(llist_node* pNode) {
	assert(pNode);
	pNode->pPreviousNode = NULL;
	pNode->pNextNode = NULL;
};

// Insert pNewNode after pNode
static inline void LinkedList_InsertAfter(llist_node* pNode, llist_node* pNewNode) {
	assert(pNode);
	assert(pNewNode);
	pNewNode->pPreviousNode = pNode;
	pNewNode->pNextNode = pNode->pNextNode;
	pNode->pNextNode = pNewNode;
}

// Insert pNewNode before pNode
static inline void LinkedList_InsertBefore(llist_node* pNode, llist_node* pNewNode) {
	assert(pNode);
	assert(pNewNode);
	pNewNode->pPreviousNode = pNode->pPreviousNode;
	pNewNode->pNextNode = pNode;
	pNode->pPreviousNode = pNewNode;
}

// Remove pNode from the list
static inline void LinkedList_Remove(llist_node* pNode) {
	assert(pNode);
	if (pNode->pPreviousNode != NULL)
		pNode->pPreviousNode->pNextNode = pNode->pNextNode;
	if (pNode->pNextNode != NULL)
		pNode->pNextNode->pPreviousNode = pNode->pPreviousNode;
}
