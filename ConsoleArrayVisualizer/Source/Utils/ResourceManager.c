
#include <stdint.h>
#include "Utils/GuardedMalloc.h"
#include "Utils/Tree234.h"
#include "Utils/ResourceManager.h"

#define INVALID_HANDLE (0) // reserved handle
#define FIRST_HANDLE (1)

typedef struct {
	handle_t Handle;
	void* pResourceData;
} handle_resource_pair;

static int HandleCmp(handle_t* pA, handle_t* pB) {
	return (*pA > *pB) - (*pA < *pB);
}

static int HandleResourcePairCmp(handle_resource_pair* pA, handle_resource_pair* pB) {
	return (pA->Handle > pB->Handle) - (pA->Handle < pB->Handle);
}

void CreateResourceTable(resource_table_t* prtTable) {

	prtTable->ptreeHandleResourcePair = newtree234((cmpfn234)HandleResourcePairCmp);
	prtTable->ptreeEmptyHandle = newtree234((cmpfn234)HandleCmp);

	handle_t* FirstHandle = malloc_guarded(sizeof(handle_t));
	*FirstHandle = FIRST_HANDLE;
	add234(prtTable->ptreeEmptyHandle, FirstHandle);

	return;

}

void DestroyResourceTable(resource_table_t* prtTable) {

	tree234* ptreeHandleResourcePair = prtTable->ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle = prtTable->ptreeEmptyHandle;

	intptr_t i = 0;
	for (
		void* p = delpos234(ptreeHandleResourcePair, 0);
		p != NULL;
		p = delpos234(ptreeHandleResourcePair, 0)
	) {
		free(p);
	}
	freetree234(ptreeHandleResourcePair);

	for (
		void* p = delpos234(ptreeEmptyHandle, 0);
		p != NULL;
		p = delpos234(ptreeEmptyHandle, 0)
	) {
		free(p);
	}
	freetree234(ptreeEmptyHandle);

	return;

}

handle_t AddResource(resource_table_t* prtTable, void* pResource) {

	tree234* ptreeHandleResourcePair = prtTable->ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle = prtTable->ptreeEmptyHandle;

	if (count234(ptreeEmptyHandle) == UINTPTR_MAX) // Out of ids
		return INVALID_HANDLE;
	intptr_t* pEmptyHandle = index234(ptreeEmptyHandle, 0);
	intptr_t NewHandle = *pEmptyHandle;

	// Update the empty chunk

	// Check if the next handle is already used
	handle_resource_pair hdpSearch = (handle_resource_pair){ NewHandle + 1, 0 };
	if (find234(ptreeHandleResourcePair, &hdpSearch, NULL) != NULL) {
		// If yes, this empty chunk is filled -> no longer empty
		del234(ptreeEmptyHandle, pEmptyHandle);
		free(pEmptyHandle);
	} else {
		// This empty chunk is not filled
		(*pEmptyHandle) += 1;
	}

	// Add the new marker to the marker list

	handle_resource_pair* phdpNewHandleResourcePair = malloc_guarded(sizeof(handle_resource_pair));
	phdpNewHandleResourcePair->Handle = NewHandle;
	phdpNewHandleResourcePair->pResourceData = pResource;
	add234(ptreeHandleResourcePair, phdpNewHandleResourcePair);

	return;

}

void* RemoveResource(resource_table_t* prtTable, handle_t Handle) {

	tree234* ptreeHandleResourcePair = prtTable->ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle = prtTable->ptreeEmptyHandle;

	// Delete from the marker list

	handle_resource_pair hdpSearch = (handle_resource_pair){ Handle, 0 };
	handle_resource_pair* phdpResult = del234(ptreeHandleResourcePair, &hdpSearch);
	if (!phdpResult)
		return NULL;

	void* pResourceData = phdpResult->pResourceData;
	free(phdpResult);

	// Check if the next handle is already used

	intptr_t NextHandle = Handle + 1;
	intptr_t PrevHandle = Handle - 1;

	hdpSearch = (handle_resource_pair){ NextHandle, 0 };
	handle_resource_pair* phdpNext = find234(ptreeHandleResourcePair, &hdpSearch, NULL);
	if (phdpNext == NULL) {

		// The next handle is not used -> an empty chunk is there
		// Decrement that empty chunk to this marker handle
		intptr_t* pNextHandle = find234(ptreeEmptyHandle, &NextHandle, NULL);
		(*pNextHandle) -= 1;

		if (Handle == FIRST_HANDLE) {

			// It's already the first handle so no previous handle

		} else {

			// Check if the previous handle is already used
			hdpSearch = (handle_resource_pair){ PrevHandle, 0 };
			handle_resource_pair* phdpPrev = find234(ptreeHandleResourcePair, &hdpSearch, NULL);
			if (phdpPrev == NULL) {

				// Empty chunks (previous & next) is merged
				del234(ptreeEmptyHandle, pNextHandle);
				free(pNextHandle);

			}

		}

	} else {

		if (Handle == FIRST_HANDLE) {

			// No need to check the previous one
			// A new empty chunk have just formed
			intptr_t* pNewEmptyHandle = malloc_guarded(sizeof(intptr_t));
			*pNewEmptyHandle = Handle;
			add234(ptreeEmptyHandle, pNewEmptyHandle);

		} else {

			// Check if the previous handle is already used
			hdpSearch = (handle_resource_pair){ PrevHandle, 0 };
			handle_resource_pair* phdpPrev = find234(ptreeHandleResourcePair, &hdpSearch, NULL);
			if (phdpPrev == NULL) {

				// The previous handle is not used
				// Already in an existing empty chunk so do nothing

			} else {

				// A new empty chunk have just formed
				intptr_t* pNewEmptyHandle = malloc_guarded(sizeof(intptr_t));
				*pNewEmptyHandle = Handle;
				add234(ptreeEmptyHandle, pNewEmptyHandle);

			}

		}

	}

	return pResourceData;

}

void* GetResource(resource_table_t* prtTable, handle_t Handle) {
	handle_resource_pair hdpSearch = (handle_resource_pair){ Handle, 0 };
	handle_resource_pair* phdpResult = find234(prtTable->ptreeHandleResourcePair, &hdpSearch, NULL);
	if (!phdpResult)
		return NULL;
	else
		return phdpResult->pResourceData;
}

void* SetResource(resource_table_t* prtTable, handle_t Handle, void* pResourceData) {
	handle_resource_pair hdpSearch = (handle_resource_pair){ Handle, 0 };
	handle_resource_pair* phdpResult = find234(prtTable->ptreeHandleResourcePair, &hdpSearch, NULL);
	void* pOldResourceData = phdpResult->pResourceData;
	phdpResult->pResourceData = pResourceData;
	return pOldResourceData;
}

intptr_t GetResourceCount(resource_table_t* prtTable) {
	return count234(prtTable->ptreeHandleResourcePair);
}

void** GetResourceList(resource_table_t* prtTable, intptr_t* pResourceCount) {
	intptr_t ResourceCount = count234(prtTable->ptreeHandleResourcePair);
	if (pResourceCount)
		*pResourceCount = ResourceCount;
	void** apResourceList = malloc(ResourceCount * sizeof(void*));
	for (intptr_t i = 0; i < ResourceCount; ++i)
		apResourceList[i] = ((handle_resource_pair*)index234(prtTable->ptreeHandleResourcePair, i))->pResourceData;
	return apResourceList;
}
