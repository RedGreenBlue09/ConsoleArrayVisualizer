
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
	Visualizer_ArrayHandle hArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_Marker;

typedef void* Visualizer_MarkerHandle;

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

	Visualizer_RendererEntry.Initialize();

	PoolInitialize(&Visualizer_ArrayPropPool, 16, sizeof(Visualizer_ArrayProp));
	PoolInitialize(&Visualizer_MarkerPool, 256, sizeof(Visualizer_Marker));
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

// Array

static int Visualizer_MarkerPriorityCmp(
	Visualizer_Marker* pMarkerA,
	Visualizer_Marker* pMarkerB
);

Visualizer_ArrayHandle Visualizer_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return NULL;
	if (Size < 1) return NULL;
	
	pool_index PoolIndex = PoolAllocate(&Visualizer_ArrayPropPool);
	if (PoolIndex >= Visualizer_ArrayPropPool.nBlock) return NULL;
	Visualizer_ArrayProp* pArrayProp = PoolGetAddress(&Visualizer_ArrayPropPool, PoolIndex);

	// Initialize apMarkerTree

	pArrayProp->Size = Size;
	pArrayProp->apMarkerTree = malloc_guarded(Size * sizeof(tree234*));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->apMarkerTree[i] = newtree234(Visualizer_MarkerPriorityCmp);

	// Call Renderer

	Visualizer_RendererEntry.AddArray((Visualizer_ArrayHandle)pArrayProp, Size, aArrayState, ValueMin, ValueMax);
	return (Visualizer_ArrayHandle)PoolIndex;

}

void Visualizer_RemoveArray(Visualizer_ArrayHandle hArray) {

	if (!Visualizer_bInitialized) return NULL;
	if (!hArray) return NULL;

	Visualizer_ArrayProp* pArrayProp = PoolGetAddress(&Visualizer_ArrayPropPool, (pool_index)hArray);
	intptr_t Size = pArrayProp->Size;

	// Uninitialize apMarkerTree

	for (intptr_t i = 0; i < Size; ++i) {
		for (
			Visualizer_Marker* pMarker = delpos234(pArrayProp->apMarkerTree[i], 0);
			pMarker != NULL;
			pMarker = delpos234(pArrayProp->apMarkerTree[i], 0)
		) {
			PoolDeallocate(&Visualizer_MarkerPool, pMarker);
		}
		freetree234(pArrayProp->apMarkerTree[i]);
	}
	free(pArrayProp->apMarkerTree);

	PoolDeallocate(&Visualizer_ArrayPropPool, (pool_index)hArray);

	// Call renderer

	Visualizer_RendererEntry.RemoveArray(hArray);
	return;

}

void Visualizer_UpdateArray(
	Visualizer_ArrayHandle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return;
	if (!hArray) return;
	if (ValueMax <= ValueMin) return;

	Visualizer_ArrayProp* pArrayProp = PoolGetAddress(&Visualizer_ArrayPropPool, (pool_index)hArray);

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
				PoolDeallocate(&Visualizer_MarkerPool, pMarker);
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
		hArray,
		NewSize,
		ValueMin,
		ValueMax
	);
	return;
}

// Marker

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

static Visualizer_MarkerHandle Visualizer_NewMarker(
	Visualizer_ArrayHandle hArray,
	intptr_t iPosition,
	uint8_t bUpdateValue,
	isort_t NewValue,
	Visualizer_MarkerAttribute Attribute
) {

	assert(Visualizer_bInitialized);
	assert(hArray);

	Visualizer_ArrayProp* pArrayProp = PoolGetAddress(&Visualizer_ArrayPropPool, (pool_index)hArray);
	assert(iPosition >= 0 && iPosition < pArrayProp->Size);

	pool_index PoolIndex = PoolAllocate(&Visualizer_MarkerPool);
	Visualizer_Marker* pMarker = PoolGetAddress(&Visualizer_MarkerPool, PoolIndex);
	pMarker->hArray = hArray;
	pMarker->iPosition = iPosition;
	pMarker->Attribute = Attribute;

	add234(pArrayProp->apMarkerTree[iPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pHighestPriorityMarker = findrel234(pArrayProp->apMarkerTree[iPosition], NULL, NULL, REL234_LT);

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;
	Visualizer_RendererEntry.UpdateItem(
		hArray,
		iPosition,
		UpdateRequest,
		NewValue,
		pHighestPriorityMarker->Attribute
	);

	return (Visualizer_MarkerHandle)PoolIndex;

}

static void Visualizer_RemoveMarker(Visualizer_MarkerHandle hMarker) {

	assert(hMarker);

	Visualizer_Marker* pMarker = PoolGetAddress(&Visualizer_MarkerPool, (pool_index)hMarker);
	Visualizer_ArrayProp* pArrayProp = PoolGetAddress(&Visualizer_ArrayPropPool, (pool_index)pMarker->hArray);

	intptr_t iPosition = pMarker->iPosition;
	del234(pArrayProp->apMarkerTree[iPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pHighestPriorityMarker = findrel234(pArrayProp->apMarkerTree[iPosition], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal

	Visualizer_MarkerAttribute TargetAttribute =
		pHighestPriorityMarker ?
		pHighestPriorityMarker->Attribute :
		Visualizer_MarkerAttribute_Normal;

	Visualizer_RendererEntry.UpdateItem(
		pMarker->hArray,
		iPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);
	PoolDeallocate(&Visualizer_MarkerPool, pMarker);

	return;
}

// On the same array & keep attribute
static void Visualizer_MoveMarker(Visualizer_MarkerHandle hMarker, intptr_t iNewPosition) {

	assert(pMarker);

	Visualizer_Marker* pMarker = PoolGetAddress(&Visualizer_MarkerPool, (pool_index)hMarker);
	Visualizer_ArrayProp* pArrayProp = PoolGetAddress(&Visualizer_ArrayPropPool, (pool_index)pMarker->hArray);
	assert(iNewPosition >= 0 && iNewPosition < pArrayProp->Size);

	// Delete from old map slot

	intptr_t iOldPosition = pMarker->iPosition;
	del234(pArrayProp->apMarkerTree[iOldPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pHighestPriorityMarker = findrel234(pArrayProp->apMarkerTree[iOldPosition], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal

	Visualizer_MarkerAttribute TargetAttribute =
		pHighestPriorityMarker ?
		pHighestPriorityMarker->Attribute :
		Visualizer_MarkerAttribute_Normal;

	Visualizer_RendererEntry.UpdateItem(
		pMarker->hArray,
		iOldPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);

	// Add to new map slot

	pMarker->iPosition = iNewPosition;
	add234(pArrayProp->apMarkerTree[iNewPosition], pMarker);

	// Get the highest priority marker

	pHighestPriorityMarker = findrel234(pArrayProp->apMarkerTree[iNewPosition], NULL, NULL, REL234_LT);

	Visualizer_RendererEntry.UpdateItem(
		pMarker->hArray,
		iNewPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		pHighestPriorityMarker->Attribute
	);

	return;
}

// Read & Write

void Visualizer_UpdateRead(Visualizer_ArrayHandle hArray, intptr_t iPosition, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!hArray) return;

	Visualizer_Marker* pMarker = Visualizer_NewMarker(
		hArray,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(pMarker);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(Visualizer_ArrayHandle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!hArray) return;

	Visualizer_Marker* pMarkerA = Visualizer_NewMarker(
		hArray,
		iPositionA,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Marker* pMarkerB = Visualizer_NewMarker(
		hArray,
		iPositionB,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(pMarkerA);
	Visualizer_RemoveMarker(pMarkerB);

	return;

}

void Visualizer_UpdateReadMulti(
	Visualizer_ArrayHandle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!hArray) return;
	if (Length < 0) return;

	Visualizer_Marker** apMarker = malloc_guarded(Length * sizeof(*apMarker));
	for (intptr_t i = 0; i < Length; ++i) {
		apMarker[i] = Visualizer_NewMarker(
			hArray,
			iStartPosition + i,
			false,
			0,
			Visualizer_MarkerAttribute_Read
		);
	}
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = iStartPosition; i < (iStartPosition + Length); ++i)
		Visualizer_RemoveMarker(apMarker[i]);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(Visualizer_ArrayHandle hArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!hArray) return;

	Visualizer_Marker* pMarker = Visualizer_NewMarker(
		hArray,
		iPosition,
		true,
		NewValue,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(pMarker);

	return;

}

void Visualizer_UpdateWrite2(
	Visualizer_ArrayHandle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!hArray) return;

	Visualizer_Marker* pMarkerA = Visualizer_NewMarker(
		hArray,
		iPositionA,
		false,
		0,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Marker* pMarkerB = Visualizer_NewMarker(
		hArray,
		iPositionB,
		false,
		0,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(pMarkerA);
	Visualizer_RemoveMarker(pMarkerB);

	return;

}

void Visualizer_UpdateWriteMulti(
	Visualizer_ArrayHandle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!hArray) return;
	if (Length < 0) return;

	Visualizer_Marker** apMarker = malloc_guarded(Length * sizeof(*apMarker));
	for (intptr_t i = 0; i < Length; ++i) {
		apMarker[i] = Visualizer_NewMarker(
			hArray,
			iStartPosition + i,
			true,
			aNewValue[i],
			Visualizer_MarkerAttribute_Write
		);
	}
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = iStartPosition; i < (iStartPosition + Length); ++i)
		Visualizer_RemoveMarker(apMarker[i]);

	return;

}

// Pointer

Visualizer_PointerHandle Visualizer_CreatePointer(
	Visualizer_ArrayHandle hArray,
	intptr_t iPosition
) {
	if (!Visualizer_bInitialized) return NULL;
	if (!hArray) return;

	return (Visualizer_PointerHandle)Visualizer_NewMarker(
		hArray,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Pointer
	);
}

void Visualizer_RemovePointer(
	Visualizer_PointerHandle hPointer
) {
	if (!Visualizer_bInitialized) return;
	if (!hPointer) return;

	Visualizer_RemoveMarker((Visualizer_Marker*)hPointer);

	return;
}

void Visualizer_MovePointer(
	Visualizer_PointerHandle hPointer,
	intptr_t iNewPosition
) {
	if (!Visualizer_bInitialized) return;
	if (!hPointer) return;

	Visualizer_MoveMarker((Visualizer_Marker*)hPointer, iNewPosition);

	return;
}

#endif