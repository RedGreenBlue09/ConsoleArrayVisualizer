
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

	if (Visualizer_bInitialized) return;

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

	if (!Visualizer_bInitialized) return;

	Visualizer_bInitialized = false;
	PoolDestroy(&Visualizer_ArrayPropPool);
	PoolDestroy(&Visualizer_MarkerPool);

	// Call renderer
	Visualizer_RendererEntry.Uninitialize();

	return;
}

#ifndef VISUALIZER_DISABLE_SLEEP

void Visualizer_Sleep(double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

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

	if (!Visualizer_bInitialized) return NULL;
	if (Size < 1) return NULL;
	
	pool_index Index = PoolAllocate(&Visualizer_ArrayPropPool);
	if (Index == POOL_INVALID_INDEX) return NULL;
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);

	// Initialize aMarkerList

	pArrayProp->aiMarkerListTail = malloc_guarded(Size * sizeof(*pArrayProp->aiMarkerListTail));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->aiMarkerListTail[i] = POOL_INVALID_INDEX;

	// Call Renderer

	Visualizer_RendererEntry.AddArray(Index, Size, aArrayState, ValueMin, ValueMax);
	return Visualizer_PoolIndexToHandle(Index);

}

void Visualizer_RemoveArray(Visualizer_Handle hArray) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;

	pool_index Index = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);
	intptr_t Size = pArrayProp->Size;

	// Uninitialize aMarkerList

	for (intptr_t i = 0; i < Size; ++i) {
		pool_index iNode = pArrayProp->aiMarkerListTail[i];
		while (iNode != POOL_INVALID_INDEX) {
			Visualizer_MarkerNode* pNode = PoolIndexToAddress(&Visualizer_MarkerPool, iNode);
			pool_index iPreviousNode = pNode->Node.iPreviousNode;
			PoolDeallocate(&Visualizer_MarkerPool, iNode);
			iNode = iPreviousNode;
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

	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;
	if (ValueMax <= ValueMin) return;

	pool_index Index = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Free unused part from the marker map

		intptr_t OldSize = pArrayProp->Size;
		for (intptr_t i = NewSize; i < OldSize; ++i) {
			pool_index iNode = pArrayProp->aiMarkerListTail[i];
			while (iNode != POOL_INVALID_INDEX) {
				Visualizer_MarkerNode* pNode = PoolIndexToAddress(&Visualizer_MarkerPool, iNode);
				pool_index iPreviousNode = pNode->Node.iPreviousNode;
				PoolDeallocate(&Visualizer_MarkerPool, iNode);
				iNode = iPreviousNode;
			}
		}

		// Resize the marker map

		pool_index* aiResizedMarkerListTail = malloc_guarded(NewSize * sizeof(*aiResizedMarkerListTail));

		// Initialize the new part

		for (intptr_t i = OldSize; i < NewSize; ++i)
			pArrayProp->aiMarkerListTail[i] = POOL_INVALID_INDEX;
		pArrayProp->aiMarkerListTail = aiResizedMarkerListTail;
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

static const int Visualizer_MarkerAttrPriority[Visualizer_MarkerAttribute_EnumCount] = {
	0, //Visualizer_MarkerAttribute_Background
	1, //Visualizer_MarkerAttribute_Normal
	3, //Visualizer_MarkerAttribute_Read
	4, //Visualizer_MarkerAttribute_Write
	2, //Visualizer_MarkerAttribute_Pointer
	5, //Visualizer_MarkerAttribute_Correct
	6, //Visualizer_MarkerAttribute_Incorrect
};

static Visualizer_Handle Visualizer_NewMarker(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute
) {

	assert(Visualizer_bInitialized);
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	pool_index iArray = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, iArray);
	assert(iPosition >= 0 && iPosition < pArrayProp->Size);

	pool_index* piTailNode = &pArrayProp->aiMarkerListTail[iPosition];
	pool_index iMarker = LinkedList_NewNode(&Visualizer_MarkerPool);

	if (iMarker != POOL_INVALID_INDEX) {
		Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
		pMarker->hArray = hArray;
		pMarker->iPosition = iPosition;
		pMarker->Attribute = Attribute;

		if (*piTailNode != POOL_INVALID_INDEX)
			LinkedList_Insert(&Visualizer_MarkerPool, *piTailNode, iMarker);
		*piTailNode = iMarker;

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

	assert(Visualizer_bInitialized);
	assert(Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker));

	pool_index iMarker = Visualizer_HandleToPoolIndex(hMarker);
	Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
	pool_index iArray = Visualizer_HandleToPoolIndex(pMarker->hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, iArray);
	intptr_t iPosition = pMarker->iPosition;
	pool_index* piTailNode = &pArrayProp->aiMarkerListTail[iPosition];

	LinkedList_Remove(&Visualizer_MarkerPool, iMarker);
	if (iMarker == *piTailNode) { // Remove tail

		*piTailNode = pMarker->Node.iPreviousNode;
		Visualizer_MarkerAttribute Attribute;
		if (*piTailNode == POOL_INVALID_INDEX) // Nothing left
			Attribute = Visualizer_MarkerAttribute_Normal;
		else {
			Visualizer_MarkerNode* pTopMarker = PoolIndexToAddress(&Visualizer_ArrayPropPool, *piTailNode);
			Attribute = pTopMarker->Attribute;
		}

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
	pool_index* piOldTailNode = &pArrayProp->aiMarkerListTail[iOldPosition];

	LinkedList_Remove(&Visualizer_MarkerPool, iMarker);
	if (iMarker == *piOldTailNode) { // Remove tail

		*piOldTailNode = pMarker->Node.iPreviousNode;
		Visualizer_MarkerAttribute Attribute;
		if (*piOldTailNode == POOL_INVALID_INDEX) // Nothing left
			Attribute = Visualizer_MarkerAttribute_Normal;
		else {
			Visualizer_MarkerNode* pTopMarker = PoolIndexToAddress(&Visualizer_ArrayPropPool, *piOldTailNode);
			Attribute = pTopMarker->Attribute;
		}

		Visualizer_RendererEntry.UpdateItem(
			iArray,
			iOldPosition,
			AV_RENDERER_UPDATEATTR,
			0,
			Attribute
		);

	} // Else no update

	// Add new position

	pool_index* piNewTailNode = &pArrayProp->aiMarkerListTail[iNewPosition];
	if (*piNewTailNode != POOL_INVALID_INDEX)
		LinkedList_Insert(&Visualizer_MarkerPool, *piNewTailNode, iMarker);
	*piNewTailNode = iMarker;

	Visualizer_MarkerNode* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, iMarker);
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

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;

	Visualizer_Handle hMarker = Visualizer_NewMarker(
		hArray,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Sleep(fSleepMultiplier);
	if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker))
		Visualizer_RemoveMarker(hMarker);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(Visualizer_Handle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;

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
	if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarkerA))
		Visualizer_RemoveMarker(hMarkerA);
	if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarkerB))
		Visualizer_RemoveMarker(hMarkerB);

	return;

}

void Visualizer_UpdateReadMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;
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
		if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, ahMarker[i]))
			Visualizer_RemoveMarker(ahMarker[i]);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(Visualizer_Handle hArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;

	Visualizer_Handle hMarker = Visualizer_NewMarker(
		hArray,
		iPosition,
		true,
		NewValue,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Sleep(fSleepMultiplier);
	if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker))
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

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;

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
	if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarkerA))
		Visualizer_RemoveMarker(hMarkerA);
	if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarkerB))
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

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return;
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
		if (Visualizer_ValidateHandle(&Visualizer_MarkerPool, ahMarker[i]))
			Visualizer_RemoveMarker(ahMarker[i]);

	return;

}

// Pointer

Visualizer_Handle Visualizer_CreatePointer(
	Visualizer_Handle hArray,
	intptr_t iPosition
) {
	if (!Visualizer_bInitialized) return NULL;
	if (!Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray)) return NULL;

	return (Visualizer_Handle)Visualizer_NewMarker(
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
	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_MarkerPool, hPointer)) return;

	Visualizer_RemoveMarker(hPointer);

	return;
}

void Visualizer_MovePointer(
	Visualizer_Handle hPointer,
	intptr_t iNewPosition
) {
	if (!Visualizer_bInitialized) return;
	if (!Visualizer_ValidateHandle(&Visualizer_MarkerPool, hPointer)) return;

	Visualizer_MoveMarker(hPointer, iNewPosition);

	return;
}

#endif