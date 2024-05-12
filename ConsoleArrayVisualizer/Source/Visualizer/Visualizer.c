
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
	Visualizer_Handle hArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_Marker;

typedef void* Visualizer_Handle;

static int Visualizer_MarkerPriorityCmp(
	Visualizer_Marker* pMarkerA,
	Visualizer_Marker* pMarkerB
);

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
	PoolInitialize(&Visualizer_MarkerPool, 16, sizeof(Visualizer_Marker));
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
	if (!PoolValidateIndex(&Visualizer_ArrayPropPool, Index)) return NULL;
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, Index);

	// Initialize apMarkerTree

	pArrayProp->Size = Size;
	pArrayProp->apMarkerTree = malloc_guarded(Size * sizeof(tree234*));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->apMarkerTree[i] = newtree234(Visualizer_MarkerPriorityCmp);

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

	// Uninitialize apMarkerTree

	for (intptr_t i = 0; i < Size; ++i) {
		for (
			Visualizer_Marker* pMarker = delpos234(pArrayProp->apMarkerTree[i], 0);
			pMarker != NULL;
			pMarker = delpos234(pArrayProp->apMarkerTree[i], 0)
		) {
			pool_index MarkerIndex = PoolAddressToIndex(&Visualizer_MarkerPool, pMarker);
			PoolDeallocate(&Visualizer_MarkerPool, MarkerIndex);
		}
		freetree234(pArrayProp->apMarkerTree[i]);
	}
	free(pArrayProp->apMarkerTree);

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
		tree234** apOldMarkerTree = pArrayProp->apMarkerTree;
		for (intptr_t i = NewSize; i < OldSize; ++i) {
			for (
				Visualizer_Marker* pMarker = delpos234(apOldMarkerTree[i], 0);
				pMarker != NULL;
				pMarker = delpos234(apOldMarkerTree[i], 0)
				) {
				pool_index MarkerIndex = PoolAddressToIndex(&Visualizer_MarkerPool, pMarker);
				PoolDeallocate(&Visualizer_MarkerPool, MarkerIndex);
			}
			freetree234(apOldMarkerTree[i]);
		}

		// Resize the marker map

		tree234** apResizedMarkerTree = realloc_guarded(
			apOldMarkerTree,
			NewSize * sizeof(tree234*)
		);

		// Initialize the new part

		for (intptr_t i = OldSize; i < NewSize; ++i)
			apResizedMarkerTree[i] = newtree234(Visualizer_MarkerPriorityCmp);
		pArrayProp->apMarkerTree = apResizedMarkerTree;
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

static const int Visualizer_MarkerAttrPriority[Visualizer_MarkerAttribute_EnumCount] = {
	0, //Visualizer_MarkerAttribute_Background
	1, //Visualizer_MarkerAttribute_Normal
	3, //Visualizer_MarkerAttribute_Read
	4, //Visualizer_MarkerAttribute_Write
	2, //Visualizer_MarkerAttribute_Pointer
	5, //Visualizer_MarkerAttribute_Correct
	6, //Visualizer_MarkerAttribute_Incorrect
};

static int Visualizer_MarkerPriorityCmp(
	Visualizer_Marker* pMarkerA,
	Visualizer_Marker* pMarkerB
) {
	uintptr_t PrioA = (uintptr_t)Visualizer_MarkerAttrPriority[pMarkerA->Attribute];
	uintptr_t PrioB = (uintptr_t)Visualizer_MarkerAttrPriority[pMarkerB->Attribute];
	uintptr_t ValueA = (PrioA == PrioB) ? (uintptr_t)pMarkerA : PrioA;
	uintptr_t ValueB = (PrioA == PrioB) ? (uintptr_t)pMarkerB : PrioB;
	return (ValueA > ValueB) - (ValueA < ValueB);
}

static Visualizer_Handle Visualizer_NewMarker(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	uint8_t bUpdateValue,
	isort_t NewValue,
	Visualizer_MarkerAttribute Attribute
) {

	assert(Visualizer_bInitialized);
	assert(Visualizer_ValidateHandle(&Visualizer_ArrayPropPool, hArray));

	pool_index ArrayIndex = Visualizer_HandleToPoolIndex(hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, ArrayIndex);
	assert(iPosition >= 0 && iPosition < pArrayProp->Size);

	pool_index MarkerIndex = PoolAllocate(&Visualizer_MarkerPool);
	Visualizer_MarkerAttribute TargetAttribute = Visualizer_MarkerAttribute_Normal;
	Visualizer_Handle hMarker = NULL;
	if (PoolValidateIndex(&Visualizer_MarkerPool, MarkerIndex)) {

		Visualizer_Marker* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, MarkerIndex);
		pMarker->hArray = hArray;
		pMarker->iPosition = iPosition;
		pMarker->Attribute = Attribute;

		add234(pArrayProp->apMarkerTree[iPosition], pMarker);

		// Get the highest priority marker

		Visualizer_Marker* pTopMarker = findrel234(pArrayProp->apMarkerTree[iPosition], NULL, NULL, REL234_LT);
		TargetAttribute = pTopMarker->Attribute;
		hMarker = Visualizer_PoolIndexToHandle(MarkerIndex);

	}

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;
	Visualizer_RendererEntry.UpdateItem(
		ArrayIndex,
		iPosition,
		UpdateRequest,
		NewValue,
		TargetAttribute
	);

	return hMarker;

}

static void Visualizer_RemoveMarker(Visualizer_Handle hMarker) {

	assert(Visualizer_bInitialized);
	assert(Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker));

	pool_index MarkerIndex = Visualizer_HandleToPoolIndex(hMarker);
	Visualizer_Marker* pMarker = PoolIndexToAddress(&Visualizer_MarkerPool, MarkerIndex);
	pool_index ArrayIndex = Visualizer_HandleToPoolIndex(pMarker->hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, ArrayIndex);

	intptr_t iPosition = pMarker->iPosition;
	del234(pArrayProp->apMarkerTree[iPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pTopMarker = findrel234(pArrayProp->apMarkerTree[iPosition], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal

	Visualizer_MarkerAttribute Attribute =
		pTopMarker ?
		pTopMarker->Attribute :
		Visualizer_MarkerAttribute_Normal;

	Visualizer_RendererEntry.UpdateItem(
		ArrayIndex,
		iPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		Attribute
	);
	PoolDeallocate(
		&Visualizer_MarkerPool,
		MarkerIndex
	);

	return;

}

// On the same array & keep attribute
static void Visualizer_MoveMarker(Visualizer_Handle hMarker, intptr_t iNewPosition) {

	assert(Visualizer_ValidateHandle(&Visualizer_MarkerPool, hMarker));

	Visualizer_Marker* pMarker = Visualizer_GetHandleData(&Visualizer_MarkerPool, hMarker);
	pool_index ArrayIndex = Visualizer_HandleToPoolIndex(pMarker->hArray);
	Visualizer_ArrayProp* pArrayProp = PoolIndexToAddress(&Visualizer_ArrayPropPool, ArrayIndex);
	assert(iNewPosition >= 0 && iNewPosition < pArrayProp->Size);

	// Delete from old map slot

	intptr_t iOldPosition = pMarker->iPosition;
	del234(pArrayProp->apMarkerTree[iOldPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pTopMarker = findrel234(pArrayProp->apMarkerTree[iOldPosition], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal

	Visualizer_MarkerAttribute Attribute =
		pTopMarker ?
		pTopMarker->Attribute :
		Visualizer_MarkerAttribute_Normal;

	Visualizer_RendererEntry.UpdateItem(
		ArrayIndex,
		iOldPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		Attribute
	);

	// Add to new map slot

	pMarker->iPosition = iNewPosition;
	add234(pArrayProp->apMarkerTree[iNewPosition], pMarker);

	// Get the highest priority marker

	pTopMarker = findrel234(pArrayProp->apMarkerTree[iNewPosition], NULL, NULL, REL234_LT);

	Visualizer_RendererEntry.UpdateItem(
		ArrayIndex,
		iNewPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		pTopMarker->Attribute
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