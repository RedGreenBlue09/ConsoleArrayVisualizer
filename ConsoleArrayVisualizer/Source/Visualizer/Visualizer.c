
#include <assert.h>
#include <string.h>

#include "Sorts.h"
#include "Visualizer/Visualizer.h"
#include "Visualizer/Renderer/ColumnWindowsConsole.h"
#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

#ifndef VISUALIZER_DISABLED

// TODO: More argument checks

static AV_RENDERER_ENTRY Visualizer_RendererEntry;

static const uint64_t Visualizer_TimeDefaultDelay = 10000; // microseconds
static bool Visualizer_bInitialized = false;

typedef struct {
	varray* pArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_Marker;

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

	Visualizer_bInitialized = true;

	return;

}

void Visualizer_Uninitialize() {

	if (!Visualizer_bInitialized) return;

	Visualizer_bInitialized = false;

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

varray* Visualizer_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return NULL;
	if (Size < 1) return NULL;
	
	Visualizer_ArrayProp* pArrayProp = malloc_guarded(sizeof(Visualizer_ArrayProp));

	pArrayProp->Size = Size;
	pArrayProp->aptreeMarkerMap = malloc_guarded(Size * sizeof(tree234*));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->aptreeMarkerMap[i] = newtree234(Visualizer_MarkerPriorityCmp);

	// Call Renderer
	Visualizer_RendererEntry.AddArray((varray*)pArrayProp, Size, aArrayState, ValueMin, ValueMax);
	return (varray*)pArrayProp;

}

void Visualizer_RemoveArray(varray* pArray) {

	if (!Visualizer_bInitialized) return NULL;
	if (!pArray) return NULL;

	Visualizer_ArrayProp* pArrayProp = (Visualizer_ArrayProp*)pArray;
	intptr_t Size = pArrayProp->Size;

	// Uninitialize aptreeMarkerMap

	for (intptr_t i = 0; i < Size; ++i) {
		for (
			Visualizer_Marker* pMarker = delpos234(pArrayProp->aptreeMarkerMap[i], 0);
			pMarker != NULL;
			pMarker = delpos234(pArrayProp->aptreeMarkerMap[i], 0)
		) {
			free(pMarker);
		}
		freetree234(pArrayProp->aptreeMarkerMap[i]);
	}
	free(pArrayProp->aptreeMarkerMap);

	// Call renderer

	Visualizer_RendererEntry.RemoveArray(pArray);
	return;

}

void Visualizer_UpdateArray(
	varray* pArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return;
	if (!pArray) return;
	if (ValueMax <= ValueMin) return;

	Visualizer_ArrayProp* pArrayProp = (Visualizer_ArrayProp*)pArray;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Free unused part from the marker map

		intptr_t OldSize = pArrayProp->Size;
		tree234** aptreeOldMarkerMap = pArrayProp->aptreeMarkerMap;
		for (intptr_t i = NewSize; i < OldSize; ++i) {
			for (
				Visualizer_Marker* pMarker = delpos234(aptreeOldMarkerMap[i], 0);
				pMarker != NULL;
				pMarker = delpos234(aptreeOldMarkerMap[i], 0)
				) {
				free(pMarker);
			}
			freetree234(aptreeOldMarkerMap[i]);
		}

		// Resize the marker map

		tree234** aptreeResizedMarkerMap = realloc_guarded(
			aptreeOldMarkerMap,
			NewSize * sizeof(tree234*)
		);

		// Initialize the new part

		for (intptr_t i = OldSize; i < NewSize; ++i)
			aptreeResizedMarkerMap[i] = newtree234(Visualizer_MarkerPriorityCmp);
		pArrayProp->aptreeMarkerMap = aptreeResizedMarkerMap;
		pArrayProp->Size = NewSize;

	}

	// Call renderer

	Visualizer_RendererEntry.UpdateArray(
		pArray,
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
	uintptr_t ValueA = (PrioA == PrioB) ? pMarkerA : PrioA;
	uintptr_t ValueB = (PrioA == PrioB) ? pMarkerB : PrioB;
	return (ValueA > ValueB) - (ValueA < ValueB);
}

static Visualizer_Marker* Visualizer_NewMarker(
	varray* pArray,
	intptr_t iPosition,
	uint8_t bUpdateValue,
	isort_t NewValue,
	Visualizer_MarkerAttribute Attribute
) {

	assert(Visualizer_bInitialized);
	assert(pArray);

	Visualizer_ArrayProp* pArrayProp = (Visualizer_ArrayProp*)pArray;
	assert(iPosition >= 0 && iPosition < pArrayProp->Size);

	// NOTE: Marker pointer is shared between resource table and map
	Visualizer_Marker* pMarker = malloc_guarded(sizeof(Visualizer_Marker));
	pMarker->pArray = pArray;
	pMarker->iPosition = iPosition;
	pMarker->Attribute = Attribute;

	add234(pArrayProp->aptreeMarkerMap[iPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pHighestPriorityMarker = findrel234(pArrayProp->aptreeMarkerMap[iPosition], NULL, NULL, REL234_LT);

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;
	Visualizer_RendererEntry.UpdateItem(
		pArray,
		iPosition,
		UpdateRequest,
		NewValue,
		pHighestPriorityMarker->Attribute
	);

	return pMarker;

}

static void Visualizer_RemoveMarker(Visualizer_Marker* pMarker) {

	assert(pMarker);

	Visualizer_ArrayProp* pArrayProp = (Visualizer_ArrayProp*)pMarker->pArray;

	intptr_t iPosition = pMarker->iPosition;
	del234(pArrayProp->aptreeMarkerMap[iPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pHighestPriorityMarker = findrel234(pArrayProp->aptreeMarkerMap[iPosition], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal

	Visualizer_MarkerAttribute TargetAttribute =
		pHighestPriorityMarker ?
		pHighestPriorityMarker->Attribute :
		Visualizer_MarkerAttribute_Normal;

	Visualizer_RendererEntry.UpdateItem(
		pMarker->pArray,
		iPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);
	free(pMarker);

	return;
}

// On the same array & keep attribute
static void Visualizer_MoveMarker(Visualizer_Marker* pMarker, intptr_t iNewPosition) {

	assert(pMarker);
	assert(iNewPosition >= 0 && iNewPosition < pArrayProp->Size);

	Visualizer_ArrayProp* pArrayProp = (Visualizer_ArrayProp*)pMarker->pArray; // TODO: Pool

	// Delete from old map slot

	intptr_t iOldPosition = pMarker->iPosition;
	del234(pArrayProp->aptreeMarkerMap[iOldPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pHighestPriorityMarker = findrel234(pArrayProp->aptreeMarkerMap[iOldPosition], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal

	Visualizer_MarkerAttribute TargetAttribute =
		pHighestPriorityMarker ?
		pHighestPriorityMarker->Attribute :
		Visualizer_MarkerAttribute_Normal;

	Visualizer_RendererEntry.UpdateItem(
		pMarker->pArray,
		iOldPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);

	// Add to new map slot

	pMarker->iPosition = iNewPosition;
	add234(pArrayProp->aptreeMarkerMap[iNewPosition], pMarker);

	// Get the highest priority marker

	pHighestPriorityMarker = findrel234(pArrayProp->aptreeMarkerMap[iNewPosition], NULL, NULL, REL234_LT);

	Visualizer_RendererEntry.UpdateItem(
		pMarker->pArray,
		iNewPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		pHighestPriorityMarker->Attribute
	);

	return;
}

// Read & Write

void Visualizer_UpdateRead(varray* pArray, intptr_t iPosition, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!pArray) return;

	Visualizer_Marker* pMarker = Visualizer_NewMarker(
		pArray,
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
void Visualizer_UpdateRead2(varray* pArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!pArray) return;

	Visualizer_Marker* pMarkerA = Visualizer_NewMarker(
		pArray,
		iPositionA,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Marker* pMarkerB = Visualizer_NewMarker(
		pArray,
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
	varray* pArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!pArray) return;
	if (Length < 0) return;

	Visualizer_Marker** apMarker = malloc_guarded(Length * sizeof(*apMarker));
	for (intptr_t i = 0; i < Length; ++i) {
		apMarker[i] = Visualizer_NewMarker(
			pArray,
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
void Visualizer_UpdateWrite(varray* pArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!pArray) return;

	Visualizer_Marker* pMarker = Visualizer_NewMarker(
		pArray,
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
	varray* pArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!pArray) return;

	Visualizer_Marker* pMarkerA = Visualizer_NewMarker(
		pArray,
		iPositionA,
		false,
		0,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Marker* pMarkerB = Visualizer_NewMarker(
		pArray,
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
	varray* pArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (!pArray) return;
	if (Length < 0) return;

	Visualizer_Marker** apMarker = malloc_guarded(Length * sizeof(*apMarker));
	for (intptr_t i = 0; i < Length; ++i) {
		apMarker[i] = Visualizer_NewMarker(
			pArray,
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

vpointer* Visualizer_CreatePointer(
	varray* pArray,
	intptr_t iPosition
) {
	if (!Visualizer_bInitialized) return NULL;
	if (!pArray) return;

	return (vpointer*)Visualizer_NewMarker(
		pArray,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Pointer
	);
}

void Visualizer_RemovePointer(
	vpointer* pPointer
) {
	if (!Visualizer_bInitialized) return;
	if (!pPointer) return;

	Visualizer_RemoveMarker((Visualizer_Marker*)pPointer);

	return;
}

void Visualizer_MovePointer(
	vpointer* pPointer,
	intptr_t iNewPosition
) {
	if (!Visualizer_bInitialized) return;
	if (!pPointer) return;

	Visualizer_MoveMarker((Visualizer_Marker*)pPointer, iNewPosition);

	return;
}

#endif