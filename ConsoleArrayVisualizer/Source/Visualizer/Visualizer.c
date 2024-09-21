
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

typedef struct {
	llist_node Node;
	Visualizer_Handle hArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_MarkerNode;

typedef void* Visualizer_Handle;

void Visualizer_Initialize() {

	assert(!Visualizer_bInitialized);

	// Only for now
	
	Visualizer_RendererEntry.Initialize   = RendererCwc_Initialize;
	Visualizer_RendererEntry.Uninitialize = RendererCwc_Uninitialize;

	Visualizer_RendererEntry.AddArray     = RendererCwc_AddArray;
	Visualizer_RendererEntry.RemoveArray  = RendererCwc_RemoveArray;
	Visualizer_RendererEntry.UpdateArray  = RendererCwc_UpdateArray;

	Visualizer_RendererEntry.UpdateItem   = RendererCwc_UpdateItem;

	/*
	Visualizer_RendererEntry.Initialize   = RendererCvt_Initialize;
	Visualizer_RendererEntry.Uninitialize = RendererCvt_Uninitialize;

	Visualizer_RendererEntry.AddArray     = RendererCvt_AddArray;
	Visualizer_RendererEntry.RemoveArray  = RendererCvt_RemoveArray;
	Visualizer_RendererEntry.UpdateArray  = RendererCvt_UpdateArray;

	Visualizer_RendererEntry.UpdateItem   = RendererCvt_UpdateItem;
	*/

	// Call renderer

	Visualizer_RendererEntry.Initialize(16);

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

	// Initialize aMarkerList

	pArrayProp->apMarkerListTail = malloc_guarded(Size * sizeof(*pArrayProp->apMarkerListTail));
	for (intptr_t i = 0; i < Size; ++i) // TODO: calloc_guarded
		pArrayProp->apMarkerListTail[i] = NULL;

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
		Visualizer_MarkerNode* pNode = pArrayProp->apMarkerListTail[i];
		while (pNode != NULL) {
			Visualizer_MarkerNode* pPreviousNode = pNode->Node.pPreviousNode;
			PoolDeallocateAddress(&Visualizer_MarkerPool, pNode);
			pNode = pPreviousNode;
		}
	}

	PoolDeallocate(
		&Visualizer_ArrayPropPool,
		Index
	);

	// Call renderer

	Visualizer_RendererEntry.RemoveArray(
		Index
	);
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
	assert(ValueMax <= ValueMin);

	pool_index Index = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Free unused part from the marker array

		intptr_t OldSize = pArrayProp->Size;
		for (intptr_t i = NewSize; i < OldSize; ++i) {
			Visualizer_MarkerNode* pNode = pArrayProp->apMarkerListTail[i];
			while (pNode != POOL_INVALID_INDEX) {
				Visualizer_MarkerNode* pPreviousNode = pNode->Node.pPreviousNode;
				PoolDeallocateAddress(&Visualizer_MarkerPool, pNode);
				pNode = pPreviousNode;
			}
		}

		// Resize the marker array

		pool_index* apResizedMarkerListTail = malloc_guarded(NewSize * sizeof(*apResizedMarkerListTail));

		// Initialize the new part

		for (intptr_t i = OldSize; i < NewSize; ++i)
			pArrayProp->apMarkerListTail[i] = NULL;
		pArrayProp->apMarkerListTail = apResizedMarkerListTail;
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

// Marker
// TODO: No highlight if time = 0
// TODO: UpdateRequest structure

static Visualizer_Handle Visualizer_NewMarker(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute
) {

	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	pool_index iArray = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, iArray);
	assert(iPosition >= 0 && iPosition < pArrayProp->Size);

	Visualizer_MarkerNode** ppTailNode = &pArrayProp->apMarkerListTail[iPosition];
	pool_index iMarker = PoolAllocate(&Visualizer_MarkerPool);

	if (iMarker != POOL_INVALID_INDEX) {
		Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
		LinkedList_InitializeNode(pMarker);
		pMarker->hArray = hArray;
		pMarker->iPosition = iPosition;
		pMarker->Attribute = Attribute;

		if (*ppTailNode != NULL)
			LinkedList_Insert(*ppTailNode, pMarker);
		*ppTailNode = pMarker;

		Visualizer_RendererEntry.UpdateItem(
			iArray,
			iPosition,
			AV_RENDERER_UPDATEATTR,
			0,
			Attribute
		);
	}

	return Visualizer_PoolIndexToHandle(iMarker);

}

static void Visualizer_RemoveMarker(Visualizer_Handle hMarker) {

	assert(Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker));

	pool_index iMarker = Visualizer_HandleToPoolIndex(hMarker);
	Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
	pool_index iArray = Visualizer_HandleToPoolIndex(pMarker->hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, iArray);
	intptr_t iPosition = pMarker->iPosition;
	Visualizer_MarkerNode** ppTailNode = &pArrayProp->apMarkerListTail[iPosition];

	LinkedList_Remove(pMarker);
	if (pMarker == *ppTailNode) { // Remove tail

		*ppTailNode = pMarker->Node.pPreviousNode;
		Visualizer_MarkerAttribute Attribute;
		if (*ppTailNode == NULL) // Nothing left
			Attribute = Visualizer_MarkerAttribute_Normal;
		else
			Attribute = (*ppTailNode)->Attribute;

		Visualizer_RendererEntry.UpdateItem(
			iArray,
			iPosition,
			AV_RENDERER_UPDATEATTR,
			0,
			Attribute
		);

	} // Else no update
	PoolDeallocate(&Visualizer_MarkerPool, iMarker);

	return;

}

// On the same array & keep attribute
static void Visualizer_MoveMarker(Visualizer_Handle hMarker, intptr_t iNewPosition) {

	assert(Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker));

	pool_index iMarker = Visualizer_HandleToPoolIndex(hMarker);
	Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
	pool_index iArray = Visualizer_HandleToPoolIndex(pMarker->hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, iArray);
	assert(iNewPosition >= 0 && iNewPosition < pArrayProp->Size);

	// Remove from old position

	intptr_t iOldPosition = pMarker->iPosition;
	Visualizer_MarkerNode** ppOldTailNode = &pArrayProp->apMarkerListTail[iOldPosition];

	LinkedList_Remove(pMarker);
	if (pMarker == *ppOldTailNode) { // Remove tail

		*ppOldTailNode = pMarker->Node.pPreviousNode;
		Visualizer_MarkerAttribute Attribute;
		if (*ppOldTailNode == NULL) // Nothing left
			Attribute = Visualizer_MarkerAttribute_Normal;
		else
			Attribute = (*ppOldTailNode)->Attribute;

		Visualizer_RendererEntry.UpdateItem(
			iArray,
			iOldPosition,
			AV_RENDERER_UPDATEATTR,
			0,
			Attribute
		);

	} // Else no update

	// Add new position

	pool_index* ppNewTailNode = &pArrayProp->apMarkerListTail[iNewPosition];
	if (*ppNewTailNode != NULL)
		LinkedList_Insert(*ppNewTailNode, pMarker);
	*ppNewTailNode = pMarker;

	Visualizer_RendererEntry.UpdateItem(
		iArray,
		iNewPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		pMarker->Attribute
	);

	return;

}

// Read & Write

void Visualizer_UpdateRead(Visualizer_Handle hArray, intptr_t iPosition, double fSleepMultiplier) {

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	Visualizer_Handle hMarker = Visualizer_NewMarker(
		hArray,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Sleep(fSleepMultiplier);
	if (hMarker) Visualizer_RemoveMarker(hMarker);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(Visualizer_Handle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	Visualizer_Handle hMarkerA = Visualizer_NewMarker(
		hArray,
		iPositionA,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Handle hMarkerB = Visualizer_NewMarker(
		hArray,
		iPositionB,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Sleep(fSleepMultiplier);
	if (hMarkerA) Visualizer_RemoveMarker(hMarkerA);
	if (hMarkerB) Visualizer_RemoveMarker(hMarkerB);

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
	if (Length < 0) return;

	Visualizer_Handle* ahMarker = malloc_guarded(Length * sizeof(*ahMarker));
	for (intptr_t i = 0; i < Length; ++i) {
		ahMarker[i] = Visualizer_NewMarker(
			hArray,
			iStartPosition + i,
			false,
			0,
			Visualizer_MarkerAttribute_Read
		);
	}
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = iStartPosition; i < (iStartPosition + Length); ++i)
		if (ahMarker[i]) Visualizer_RemoveMarker(ahMarker[i]);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(Visualizer_Handle hArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	Visualizer_Handle hMarker = Visualizer_NewMarker(
		hArray,
		iPosition,
		true,
		NewValue,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Sleep(fSleepMultiplier);
	if (hMarker) Visualizer_RemoveMarker(hMarker);

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

	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	Visualizer_Handle hMarkerA = Visualizer_NewMarker(
		hArray,
		iPositionA,
		true,
		NewValueA,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Handle hMarkerB = Visualizer_NewMarker(
		hArray,
		iPositionB,
		true,
		NewValueB,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Sleep(fSleepMultiplier);
	if (hMarkerA) Visualizer_RemoveMarker(hMarkerA);
	if (hMarkerB) Visualizer_RemoveMarker(hMarkerB);

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
	if (Length < 0) return;

	Visualizer_Handle* ahMarker = malloc_guarded(Length * sizeof(*ahMarker));
	for (intptr_t i = 0; i < Length; ++i) {
		ahMarker[i] = Visualizer_NewMarker(
			hArray,
			iStartPosition + i,
			true,
			aNewValue[i],
			Visualizer_MarkerAttribute_Write
		);
	}
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = iStartPosition; i < (iStartPosition + Length); ++i)
		if (ahMarker[i]) Visualizer_RemoveMarker(ahMarker[i]);

	return;

}

// Pointer

Visualizer_Handle Visualizer_CreatePointer(
	Visualizer_Handle hArray,
	intptr_t iPosition
) {
	assert(Visualizer_bInitialized);
	if (!hArray) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	return Visualizer_NewMarker(
		hArray,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Pointer
	);
}

void Visualizer_RemovePointer(
	Visualizer_Handle hPointer
) {
	assert(Visualizer_bInitialized);
	if (!hPointer) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hPointer));

	Visualizer_RemoveMarker(hPointer);
	return;
}

void Visualizer_MovePointer(
	Visualizer_Handle hPointer,
	intptr_t iNewPosition
) {
	assert(Visualizer_bInitialized);
	if (!hPointer) return;
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hPointer));

	Visualizer_MoveMarker(hPointer, iNewPosition);
	return;
}

#endif