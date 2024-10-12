
#include <assert.h>
#include <string.h>

#include "Sorts.h"
#include "Visualizer/Visualizer.h"
#include "Visualizer/Renderer/ColumnWindowsConsole.h"
#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

#ifndef VISUALIZER_DISABLED

// TODO: More parameter checks

static AV_RENDERER_ENTRY Visualizer_RendererEntry;
static const uint64_t Visualizer_TimeDefaultDelay = 10000; // microseconds
static bool Visualizer_bInitialized = false;
static pool Visualizer_ArrayPropPool;
static pool Visualizer_MarkerPool;

typedef void* Visualizer_Handle;

void Visualizer_Initialize() {

	assert(!Visualizer_bInitialized);

	// Only for now
	
	Visualizer_RendererEntry.Initialize   = RendererCwc_Initialize;
	Visualizer_RendererEntry.Uninitialize = RendererCwc_Uninitialize;

	Visualizer_RendererEntry.AddArray     = RendererCwc_AddArray;
	Visualizer_RendererEntry.RemoveArray  = RendererCwc_RemoveArray;
	Visualizer_RendererEntry.UpdateArray  = RendererCwc_UpdateArray;

	/*
	Visualizer_RendererEntry.Initialize   = RendererCvt_Initialize;
	Visualizer_RendererEntry.Uninitialize = RendererCvt_Uninitialize;

	Visualizer_RendererEntry.AddArray     = RendererCvt_AddArray;
	Visualizer_RendererEntry.RemoveArray  = RendererCvt_RemoveArray;
	Visualizer_RendererEntry.UpdateArray  = RendererCvt_UpdateArray;
	*/

	// Call renderer

	Visualizer_RendererEntry.Initialize(16);

	// TODO: Configurable number of markers
	PoolInitialize(&Visualizer_ArrayPropPool, 16, sizeof(Visualizer_ArrayProp));
	PoolInitialize(&Visualizer_MarkerPool, 256, sizeof(Visualizer_MarkerNode));
	Visualizer_bInitialized = true;

	return;

}

void Visualizer_Uninitialize() {

	assert(Visualizer_bInitialized);

	Visualizer_bInitialized = false;
	PoolDestroy(&Visualizer_ArrayPropPool);
	PoolDestroy(&Visualizer_MarkerPool);

	// Call renderer
	Visualizer_RendererEntry.Uninitialize();

	return;
}

#ifndef VISUALIZER_DISABLE_SLEEP

void Visualizer_Sleep(double fSleepMultiplier) {
	assert(Visualizer_bInitialized);
	sleep64((uint64_t)((double)Visualizer_TimeDefaultDelay * fSleepMultiplier));
	return;
}

#endif

// Handle

// TODO: Asserts
// FIXME: Unused parameters

static Visualizer_Handle Visualizer_PoolIndexToHandle(pool_index PoolIndex)  {
	return (Visualizer_Handle)(PoolIndex + 1);
}

static pool_index Visualizer_HandleToPoolIndex(Visualizer_Handle hHandle)  {
	return (pool_index)hHandle - 1;
}

static void* Visualizer_GetHandleData(pool* pPool, Visualizer_Handle hHandle) {
	return PoolIndexToAddress(pPool, Visualizer_HandleToPoolIndex(hHandle));
}

static bool Visualizer_ValidateHandle(pool* pPool, Visualizer_Handle hHandle) {
	return ((pool_index)hHandle > 0) & ((pool_index)hHandle <= pPool->nBlock);
}

// Array

Visualizer_Handle Visualizer_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {

	assert(Visualizer_bInitialized);
	if (Size < 1) return NULL; // TODO: Allow this
	
	pool_index Index = PoolAllocate(&Visualizer_ArrayPropPool);
	if (Index == POOL_INVALID_INDEX) return NULL;
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);

	pArrayProp->Size = Size;

	// Initialize aMarkerList

	pArrayProp->aState = calloc_guarded(Size, sizeof(*pArrayProp->aState));

	// Call Renderer

	Visualizer_RendererEntry.AddArray(Index, Size, aArrayState, ValueMin, ValueMax);
	return Visualizer_PoolIndexToHandle(Index);

}

void Visualizer_RemoveArray(Visualizer_Handle hArray) {

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	pool_index Index = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);
	intptr_t Size = pArrayProp->Size;

	// Uninitialize aMarkerList

	for (intptr_t i = 0; i < Size; ++i) {
		Visualizer_MarkerNode* pNode = pArrayProp->aState[i].pMarkerListTail;
		while (pNode != NULL) {
			Visualizer_MarkerNode* pPreviousNode = (Visualizer_MarkerNode*)pNode->Node.pPreviousNode;
			PoolDeallocateAddress(&Visualizer_MarkerPool, pNode);
			pNode = pPreviousNode;
		}
	}

	PoolDeallocate(
		&Visualizer_ArrayPropPool,
		Index
	);

	// Call renderer

	Visualizer_RendererEntry.RemoveArray(Index);
	return;

}

void Visualizer_UpdateArray(
	Visualizer_Handle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));
	assert(ValueMax >= ValueMin);

	pool_index Index = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Free unused part from the marker array

		intptr_t OldSize = pArrayProp->Size;
		for (intptr_t i = NewSize; i < OldSize; ++i) {
			Visualizer_MarkerNode* pNode = pArrayProp->aState[i].pMarkerListTail;
			while (pNode != NULL) {
				Visualizer_MarkerNode* pPreviousNode = (Visualizer_MarkerNode*)pNode->Node.pPreviousNode;
				PoolDeallocateAddress(&Visualizer_MarkerPool, pNode);
				pNode = pPreviousNode;
			}
		}

		// Resize the state array

		Visualizer_ArrayMember* aResizedState = realloc_guarded(pArrayProp->aState, NewSize * sizeof(*pArrayProp->aState));

		// Initialize the new part

		if (NewSize > OldSize)
			memset(aResizedState + NewSize - OldSize, 1, NewSize - OldSize);
		pArrayProp->aState = aResizedState;
		pArrayProp->Size = NewSize;

	}

	// Call renderer

	Visualizer_RendererEntry.UpdateArray(
		Index,
		NewSize,
		ValueMin,
		ValueMax
	);
	return;
}

//

typedef struct {
	Visualizer_UpdateType UpdateType;
	Visualizer_MarkerAttribute Attribute;
	isort_t Value;
} Visualizer_UpdateRequest;

// Update item
static inline void UpdateItem(Visualizer_SharedArrayMember *pSharedMember, Visualizer_UpdateRequest* pUpdateRequest) {
	if (pUpdateRequest->UpdateType != Visualizer_UpdateType_NoUpdate)
	{
		spinlock_lock(&pSharedMember->Lock);
		pSharedMember->bUpdated = true;
		if (pUpdateRequest->UpdateType & Visualizer_UpdateType_UpdateAttr)
			pSharedMember->Attribute = pUpdateRequest->Attribute;
		if (pUpdateRequest->UpdateType & Visualizer_UpdateType_UpdateValue)
			pSharedMember->Value = pUpdateRequest->Value;
		spinlock_unlock(&pSharedMember->Lock);
	}
}

// Marker
// TODO: No highlight if time = 0

static Visualizer_Handle Visualizer_NewMarker(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute
) {
	pool_index iArray = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, iArray);
	assert(iPosition >= 0 && iPosition < pArrayProp->Size);

	Visualizer_MarkerNode** ppTailNode = &pArrayProp->aState[iPosition].pMarkerListTail;
	pool_index iMarker = PoolAllocate(&Visualizer_MarkerPool);

	if (iMarker != POOL_INVALID_INDEX) {
		Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
		LinkedList_InitializeNode(&pMarker->Node);
		pMarker->iArray = iArray;
		pMarker->iPosition = iPosition;
		pMarker->Attribute = Attribute;

		if (*ppTailNode != NULL)
			LinkedList_Insert(&(*ppTailNode)->Node, &pMarker->Node);
		*ppTailNode = pMarker;
	}

	Visualizer_UpdateRequest UpdateRequest;
	UpdateRequest.UpdateType = Visualizer_UpdateType_UpdateAttr;
	UpdateRequest.Attribute = Attribute;
	UpdateItem(&pArrayProp->aState[iPosition].Shared, &UpdateRequest);

	return Visualizer_PoolIndexToHandle(iMarker);
}

static Visualizer_Handle Visualizer_NewMarkerAndValue(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute,
	isort_t Value
) {
	pool_index iArray = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, iArray);
	assert(iPosition >= 0 && iPosition < pArrayProp->Size);

	Visualizer_MarkerNode** ppTailNode = &pArrayProp->aState[iPosition].pMarkerListTail;
	pool_index iMarker = PoolAllocate(&Visualizer_MarkerPool);

	if (iMarker != POOL_INVALID_INDEX) {
		Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
		LinkedList_InitializeNode(&pMarker->Node);
		pMarker->iArray = iArray;
		pMarker->iPosition = iPosition;
		pMarker->Attribute = Attribute;

		if (*ppTailNode != NULL)
			LinkedList_Insert(&(*ppTailNode)->Node, &pMarker->Node);
		*ppTailNode = pMarker;
	}

	Visualizer_UpdateRequest UpdateRequest;
	UpdateRequest.UpdateType = Visualizer_UpdateType_UpdateAttr | Visualizer_UpdateType_UpdateValue;
	UpdateRequest.Attribute = Attribute;
	UpdateRequest.Value = Value;
	UpdateItem(&pArrayProp->aState[iPosition].Shared, &UpdateRequest);

	return Visualizer_PoolIndexToHandle(iMarker);
}


static void Visualizer_RemoveMarker(Visualizer_Handle hMarker) {

	if (!hMarker) return;
	assert(Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker));

	pool_index iMarker = Visualizer_HandleToPoolIndex(hMarker);
	Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, pMarker->iArray);
	Visualizer_MarkerNode** ppTailNode = &pArrayProp->aState[pMarker->iPosition].pMarkerListTail;

	LinkedList_Remove(&pMarker->Node);
	if (pMarker == *ppTailNode) { // Remove tail

		*ppTailNode = (Visualizer_MarkerNode*)pMarker->Node.pPreviousNode;

		Visualizer_UpdateRequest UpdateRequest;
		UpdateRequest.UpdateType = Visualizer_UpdateType_UpdateAttr;
		if (*ppTailNode == NULL) // Nothing left
			UpdateRequest.Attribute = Visualizer_MarkerAttribute_Normal;
		else
			UpdateRequest.Attribute = (*ppTailNode)->Attribute;
		UpdateItem(&pArrayProp->aState[pMarker->iPosition].Shared, &UpdateRequest);

	} // Else no update
	PoolDeallocate(&Visualizer_MarkerPool, iMarker);

	return;

}

// On the same array & keep attribute
static void Visualizer_MoveMarker(Visualizer_Handle hMarker, intptr_t iNewPosition) {

	if (!hMarker) return;
	assert(Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker));

	pool_index iMarker = Visualizer_HandleToPoolIndex(hMarker);
	Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, pMarker->iArray);

	// Remove from old position

	Visualizer_MarkerNode** ppOldTailNode = &pArrayProp->aState[pMarker->iPosition].pMarkerListTail;
	LinkedList_Remove(&pMarker->Node);
	if (pMarker == *ppOldTailNode) { // Remove tail

		*ppOldTailNode = (Visualizer_MarkerNode*)pMarker->Node.pPreviousNode;

		Visualizer_UpdateRequest UpdateRequest;
		UpdateRequest.UpdateType = Visualizer_UpdateType_UpdateAttr;
		if (*ppOldTailNode == NULL) // Nothing left
			UpdateRequest.Attribute = Visualizer_MarkerAttribute_Normal;
		else
			UpdateRequest.Attribute = (*ppOldTailNode)->Attribute;
		UpdateItem(&pArrayProp->aState[pMarker->iPosition].Shared, &UpdateRequest);

	} // Else no update

	// Add new position

	pMarker->iPosition = iNewPosition;
	Visualizer_MarkerNode** ppNewTailNode = &pArrayProp->aState[iNewPosition].pMarkerListTail;
	if (*ppNewTailNode != NULL)
		LinkedList_Insert(&(*ppNewTailNode)->Node, &pMarker->Node);
	*ppNewTailNode = pMarker;
	
	Visualizer_UpdateRequest UpdateRequest;
	UpdateRequest.UpdateType = Visualizer_UpdateType_UpdateAttr;
	UpdateRequest.Attribute  = pMarker->Attribute; // pMarker is the top one
	UpdateItem(&pArrayProp->aState[iNewPosition].Shared, &UpdateRequest);

	return;

}

// Read & Write

void Visualizer_UpdateRead(Visualizer_Handle hArray, intptr_t iPosition, double fSleepMultiplier) {

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	Visualizer_Handle hMarker = Visualizer_NewMarker(hArray, iPosition, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarker);
	return;
}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(Visualizer_Handle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {
	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	Visualizer_Handle hMarkerA = Visualizer_NewMarker(hArray, iPositionA, Visualizer_MarkerAttribute_Read);
	Visualizer_Handle hMarkerB = Visualizer_NewMarker(hArray, iPositionB, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarkerA);
	Visualizer_RemoveMarker(hMarkerB);
	return;
}

void Visualizer_UpdateReadMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {
	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));
	assert(Length > 16);

	Visualizer_Handle ahMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		ahMarker[i] = Visualizer_NewMarker(hArray, iStartPosition + i, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		Visualizer_RemoveMarker(ahMarker[i]);
	return;
}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(Visualizer_Handle hArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	Visualizer_Handle hMarker = Visualizer_NewMarkerAndValue(hArray, iPosition, Visualizer_MarkerAttribute_Write, NewValue);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarker);
	return;

}

void Visualizer_UpdateWrite2(
	Visualizer_Handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {
	Visualizer_Handle hMarkerA = Visualizer_NewMarkerAndValue(hArray, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	Visualizer_Handle hMarkerB = Visualizer_NewMarkerAndValue(hArray, iPositionB, Visualizer_MarkerAttribute_Write, NewValueB);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarkerA);
	Visualizer_RemoveMarker(hMarkerB);
	return;
}

void Visualizer_UpdateWriteMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
) {
	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));
	assert(Length > 16);

	Visualizer_Handle ahMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		ahMarker[i] = Visualizer_NewMarkerAndValue(hArray, iStartPosition + i, Visualizer_MarkerAttribute_Write, aNewValue[i]);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		Visualizer_RemoveMarker(ahMarker[i]);

	free(ahMarker);
	return;
}

// Pointer

Visualizer_Handle Visualizer_CreatePointer(Visualizer_Handle hArray, intptr_t iPosition) {
	assert(Visualizer_bInitialized);
	if (!hArray) return NULL;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));
	return Visualizer_NewMarker(hArray, iPosition, Visualizer_MarkerAttribute_Pointer);
}

void Visualizer_RemovePointer(Visualizer_Handle hPointer) {
	assert(Visualizer_bInitialized);
	Visualizer_RemoveMarker(hPointer);
	return;
}

void Visualizer_MovePointer(Visualizer_Handle hPointer, intptr_t iNewPosition) {
	assert(Visualizer_bInitialized);
	Visualizer_MoveMarker(hPointer, iNewPosition);
	return;
}

#endif