
#include <stdint.h>
#include "Utils/GuardedMalloc.h"
#include "Utils/Tree234.h"
#include "Utils/ResourceManager.h"

typedef struct {
	rm_handle_t Handle;
	void* pResourceData;
} handle_resource_pair;

static int HandleCmp(rm_handle_t* pA, rm_handle_t* pB) {
	return (*pA > *pB) - (*pA < *pB);
}

static int HandleResourcePairCmp(handle_resource_pair* pA, handle_resource_pair* pB) {
	return (pA->Handle > pB->Handle) - (pA->Handle < pB->Handle);
}

void CreateResourceTable(resource_table_t* pResourceTable) {

	pResourceTable->ptreeHandleResourcePair = newtree234((cmpfn234)HandleResourcePairCmp);
	pResourceTable->ptreeEmptyHandle = newtree234((cmpfn234)HandleCmp);

	rm_handle_t* FirstHandle = malloc_guarded(sizeof(rm_handle_t));
	*FirstHandle = FIRST_HANDLE;
	add234(pResourceTable->ptreeEmptyHandle, FirstHandle);

	return;

}

void DestroyResourceTable(resource_table_t* pResourceTable) {

	tree234* ptreeHandleResourcePair = pResourceTable->ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle = pResourceTable->ptreeEmptyHandle;

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

rm_handle_t AddResource(resource_table_t* pResourceTable, void* pResource) {

	tree234* ptreeHandleResourcePair = pResourceTable->ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle = pResourceTable->ptreeEmptyHandle;

	if (count234(ptreeEmptyHandle) == UINTPTR_MAX) // Out of ids
		return INVALID_HANDLE;
	rm_handle_t* pEmptyHandle = index234(ptreeEmptyHandle, 0);
	rm_handle_t NewHandle = *pEmptyHandle;

	// Update the empty chunk

	// Check if the next handle is already used
	handle_resource_pair SearchHandleResourcePair = (handle_resource_pair){ NewHandle + 1, 0 };
	if (find234(ptreeHandleResourcePair, &SearchHandleResourcePair, NULL) != NULL) {
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

	return NewHandle;

}

void* RemoveResource(resource_table_t* pResourceTable, rm_handle_t Handle) {

	tree234* ptreeHandleResourcePair = pResourceTable->ptreeHandleResourcePair;
	tree234* ptreeEmptyHandle = pResourceTable->ptreeEmptyHandle;

	// Delete from the marker list

	handle_resource_pair SearchHandleResourcePair = (handle_resource_pair){ Handle, 0 };
	handle_resource_pair* pResultHandleResourcePair = del234(ptreeHandleResourcePair, &SearchHandleResourcePair);
	if (!pResultHandleResourcePair)
		return NULL;

	void* pResourceData = pResultHandleResourcePair->pResourceData;
	free(pResultHandleResourcePair);

	// Check if the next handle is already used

	intptr_t NextHandle = Handle + 1;
	intptr_t PrevHandle = Handle - 1;

	SearchHandleResourcePair = (handle_resource_pair){ NextHandle, 0 };
	handle_resource_pair* pNextHandleResourcePair = find234(ptreeHandleResourcePair, &SearchHandleResourcePair, NULL);
	if (pNextHandleResourcePair == NULL) {

		// The next handle is not used -> an empty chunk is there
		// Decrement that empty chunk to this marker handle
		intptr_t* pNextHandle = find234(ptreeEmptyHandle, &NextHandle, NULL);
		(*pNextHandle) -= 1;

		if (Handle == FIRST_HANDLE) {

			// It's already the first handle so no previous handle

		} else {

			// Check if the previous handle is already used
			SearchHandleResourcePair = (handle_resource_pair){ PrevHandle, 0 };
			handle_resource_pair* phdpPrev = find234(ptreeHandleResourcePair, &SearchHandleResourcePair, NULL);
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
			SearchHandleResourcePair = (handle_resource_pair){ PrevHandle, 0 };
			handle_resource_pair* phdpPrev = find234(ptreeHandleResourcePair, &SearchHandleResourcePair, NULL);
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

void* GetResource(resource_table_t* pResourceTable, rm_handle_t Handle) {
	handle_resource_pair SearchHandleResourcePair = (handle_resource_pair){ Handle, 0 };
	handle_resource_pair* pResultHandleResourcePair = find234(pResourceTable->ptreeHandleResourcePair, &SearchHandleResourcePair, NULL);
	if (!pResultHandleResourcePair)
		return NULL;
	else
		return pResultHandleResourcePair->pResourceData;
}

void* SetResource(resource_table_t* pResourceTable, rm_handle_t Handle, void* pResourceData) {
	handle_resource_pair SearchHandleResourcePair = (handle_resource_pair){ Handle, 0 };
	handle_resource_pair* pResultHandleResourcePair = find234(pResourceTable->ptreeHandleResourcePair, &SearchHandleResourcePair, NULL);
	void* pOldResourceData = pResultHandleResourcePair->pResourceData;
	pResultHandleResourcePair->pResourceData = pResourceData;
	return pOldResourceData;
}

intptr_t GetResourceCount(resource_table_t* pResourceTable) {
	return count234(pResourceTable->ptreeHandleResourcePair);
}

void** GetResourceList(resource_table_t* pResourceTable, intptr_t* pResourceCount) {
	intptr_t ResourceCount = count234(pResourceTable->ptreeHandleResourcePair);
	if (pResourceCount)
		*pResourceCount = ResourceCount;
	void** apResourceList = malloc(ResourceCount * sizeof(void*));
	for (intptr_t i = 0; i < ResourceCount; ++i)
		apResourceList[i] = ((handle_resource_pair*)index234(pResourceTable->ptreeHandleResourcePair, i))->pResourceData;
	return apResourceList;
}
