
/*
 * 
 * Code I wrote from a long time ago...
 * 
 * Circular dynamic array that achieves:
 *  + O(1) lookup
 *  + O(1) for both stack and queue operations.
 *  + No memory fragments
 * Cons:
 *  + O(N) for insertion & deletion at the middle.
 * 
 * Not heavily tested, may break anytime.
 * 
 */

#include <stdlib.h>
#include <string.h>
#include "Utils/GuardedMalloc.h"
#include "Utils/DynamicArray.h"

#define DA_INITIAL_SIZE 16

void DaCreate(DYNAMIC_ARRAY* pDa, size_t itemSize) {

	if (!pDa) abort();

	pDa->array     = malloc_guarded(DA_INITIAL_SIZE * itemSize);
	pDa->itemSize  = itemSize;
	pDa->memSize   = DA_INITIAL_SIZE;
	pDa->start     = 0;
	pDa->size      = 0;

	if (pDa->array) abort();

	return;
}

void DaDelete(DYNAMIC_ARRAY* pDa) {

	if (!pDa) abort();

	free(pDa->array);
	pDa->array    = 0;
	pDa->itemSize = 0;
	pDa->memSize  = 0;
	pDa->start    = 0;
	pDa->size     = 0;

	return;
}

size_t DaSize(DYNAMIC_ARRAY* pDa) {

	if (!pDa) abort();
	return pDa->size;
}

void* DaIndex(DYNAMIC_ARRAY* pDa, size_t index) {

	if (!pDa) abort();

	void* pDestItem;
	uint8_t* byteArray  = pDa->array;
	size_t itemSize = pDa->itemSize;
	size_t memSize  = pDa->memSize;
	size_t start    = pDa->start;

	size_t daIndex = (start + index) % memSize;
	pDestItem = byteArray + (daIndex * itemSize);

	return pDestItem;
}

void DaResize(DYNAMIC_ARRAY* pDa, size_t newMemSize) {

	if (!pDa) abort();

	void* array      = pDa->array;
	size_t itemSize  = pDa->itemSize;
	size_t memSize   = pDa->memSize;
	size_t start     = pDa->start;
	size_t size      = pDa->size;

	void* newArray = malloc_guarded(newMemSize * itemSize);

	if (!newArray) abort();

	/* Copy */

	// Discard stuff that does not fit.
	if (size > newMemSize)
		size = newMemSize;

	// Looping around is simpler but slower.
	if (start + size > memSize) {

		// size - ((start + size) % memSize)
		size_t cpySize1 = memSize - start;

		memcpy(
			newArray,
			(void*)((size_t)array + (start * itemSize)),
			cpySize1 * itemSize
		);

		memcpy(
			(void*)((size_t)newArray + (cpySize1 * itemSize)),
			array,
			(size - cpySize1) * itemSize
		);

	} else {

		memcpy(
			newArray,
			(void*)((size_t)array + (start * itemSize)),
			size * itemSize
		);
	}

	free(array);

	array = newArray;
	memSize = newMemSize;
	start = 0;

	pDa->array     = array;
	pDa->memSize   = memSize;
	pDa->itemSize  = itemSize;
	pDa->start     = start;
	pDa->size      = size;
}

void DaPushEnd(DYNAMIC_ARRAY* pDa, void* pItem) {

	if (!pDa || !pItem) abort();

	size_t itemSize  = pDa->itemSize;

	if (pDa->size + 1 > pDa->memSize)
		DaResize(pDa, pDa->memSize * 2);

	size_t pushIndex = (pDa->start + pDa->size) % pDa->memSize;
	memcpy(
		(void*)((size_t)pDa->array + (pushIndex * itemSize)),
		pItem,
		itemSize
	);

	pDa->size += 1;

	return;
}

void DaPushBegin(DYNAMIC_ARRAY* pDa, void* pItem) {

	if (!pDa || !pItem) abort();

	size_t itemSize = pDa->itemSize;

	if (pDa->size + 1 > pDa->memSize)
		DaResize(pDa, pDa->memSize * 2);

	size_t pushIndex = (pDa->memSize + pDa->start - 1) % pDa->memSize;
	memcpy(
		(void*)((size_t)pDa->array + (pushIndex * itemSize)),
		pItem,
		itemSize
	);

	pDa->start = pushIndex;
	pDa->size += 1;

	return;
}

void DaPopEnd(DYNAMIC_ARRAY* pDa, void* pItem) {

	if (!pDa || !pItem) abort();

	size_t itemSize = pDa->itemSize;

	size_t popIndex = (pDa->start + pDa->size) % pDa->memSize;
	memcpy(
		pItem,
		(void*)((size_t)pDa->array + (popIndex * itemSize)),
		itemSize
	);

	pDa->size -= 1;

	if ((pDa->memSize >= DA_INITIAL_SIZE * 2) && (pDa->size < pDa->memSize / 4))
		DaResize(pDa, pDa->memSize / 2);

	return;
}

void DaPopBegin(DYNAMIC_ARRAY* pDa, void* pItem) {

	if (!pDa || !pItem) abort();
	
	size_t itemSize = pDa->itemSize;

	size_t popIndex = (pDa->start + 1) % pDa->memSize;
	memcpy(
		pItem,
		(void*)((size_t)pDa->array + (popIndex * itemSize)),
		itemSize
	);

	if ((pDa->memSize >= DA_INITIAL_SIZE * 2) && (pDa->size < pDa->memSize / 4))
		DaResize(pDa, pDa->memSize / 2);

	pDa->start = popIndex;
	pDa->size -= 1;

	return;
}
