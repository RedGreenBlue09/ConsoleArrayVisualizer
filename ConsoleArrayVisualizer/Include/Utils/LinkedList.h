#pragma once

#include <stdint.h>

typedef struct llist_node_tag {
	struct llist_node_tag* pPreviousNode; // Slow indeed, but this is to support resizable pool
	struct llist_node_tag* pNextNode;
} llist_node;

// Create an alone node
void LinkedList_InitializeNode(llist_node* pNode);

// Insert another node after iNode
void LinkedList_Insert(llist_node* pNode, llist_node* pNewNode);

// Remove iNode from the list
void LinkedList_Remove(llist_node* pNode);
