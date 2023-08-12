
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

static resource_table_t Visualizer_ArrayPropResourceTable; // Resource table of Visualizer_ArrayProp
static resource_table_t Visualizer_MarkerResourceTable; // Resource table of Visualizer_Marker

typedef struct {
	rm_handle_t ArrayHandle;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_Marker;

typedef struct {
	rm_handle_t Handle;
	Visualizer_Marker Marker;
} Visualizer_Marker_handle_pair;


void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	// Initialize Visualizer_ArrayPropResourceTable

	CreateResourceTable(&Visualizer_ArrayPropResourceTable);

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

	// Uninitialize Visualizer_ArrayPropResourceTable

	intptr_t nRemainingArrayProp;
	Visualizer_ArrayProp** apRemainingArrayProp = (Visualizer_ArrayProp**)GetResourceList(&Visualizer_ArrayPropResourceTable, &nRemainingArrayProp);
	for (intptr_t i = 0; i < nRemainingArrayProp; ++i)
		free(apRemainingArrayProp[i]);
	free(apRemainingArrayProp);

	DestroyResourceTable(&Visualizer_ArrayPropResourceTable);

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

static int Visualizer_MarkerPriorityCmp(void* pA, void* pB);

rm_handle_t Visualizer_AddArray(intptr_t Size) {

	if (!Visualizer_bInitialized) return INVALID_HANDLE;
	if (Size < 1) return INVALID_HANDLE;
	
	//

	Visualizer_ArrayProp* pArrayProp = malloc_guarded(sizeof(Visualizer_ArrayProp));

	pArrayProp->Size = Size;
	pArrayProp->aptreeMarkerMap = malloc_guarded(Size * sizeof(tree234*));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->aptreeMarkerMap[i] = newtree234(Visualizer_MarkerPriorityCmp);

	//
	rm_handle_t ArrayHandle = AddResource(&Visualizer_ArrayPropResourceTable, pArrayProp);

	// Call Renderer
	Visualizer_RendererEntry.AddArray(ArrayHandle, Size);

	return ArrayHandle;

}

void Visualizer_RemoveArray(rm_handle_t ArrayHandle) {

	if (!Visualizer_bInitialized) return;

	// Delete from resource table

	Visualizer_ArrayProp* pArrayProp = RemoveResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return;
	intptr_t Size = pArrayProp->Size;

	// Uninitialize aptreeMarkerMap

	for (intptr_t i = 0; i < Size; ++i) {
		for (
			void* p = delpos234(pArrayProp->aptreeMarkerMap[i], 0);
			p != NULL;
			p = delpos234(pArrayProp->aptreeMarkerMap[i], 0)
		) {
			free(p);
		}
		freetree234(pArrayProp->aptreeMarkerMap[i]);
	}
	free(pArrayProp->aptreeMarkerMap);

	// Call renderer

	Visualizer_RendererEntry.RemoveArray(ArrayHandle);

	return;

}

void Visualizer_UpdateArray(
	intptr_t ArrayHandle,
	isort_t NewSize,
	isort_t* aNewArrayState,
	int32_t bVisible,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return;
	if (ValueMax <= ValueMin) return;

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pArrayProp->Size)) {

		// Resize the marker map
		// TODO: MEMORY LEAK

		tree234** aptreeResizedUmMap = realloc_guarded(pArrayProp->aptreeMarkerMap, NewSize);
		intptr_t OldSize = pArrayProp->Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aptreeResizedUmMap[OldSize + i] = newtree234(Visualizer_MarkerPriorityCmp);
		pArrayProp->aptreeMarkerMap = aptreeResizedUmMap;
		pArrayProp->Size = NewSize;

	}

	// Call renderer

	Visualizer_RendererEntry.UpdateArray(
		ArrayHandle,
		NewSize,
		aNewArrayState,
		bVisible,
		ValueMin,
		ValueMax
	);

	return;
}

// Marker

static const int Visualizer_MarkerAttrPriority[] = {
	0, //Visualizer_MarkerAttribute_Background
	1, //Visualizer_MarkerAttribute_Normal
	3, //Visualizer_MarkerAttribute_Read
	4, //Visualizer_MarkerAttribute_Write
	2, //Visualizer_MarkerAttribute_Pointer
	5, //Visualizer_MarkerAttribute_Correct
	6, //Visualizer_MarkerAttribute_Incorrect
};

static int Visualizer_MarkerPriorityCmp(void* pA, void* pB) {
	// Compare by attribute
	Visualizer_Marker* A = pA;
	Visualizer_Marker* B = pB;
	int PrioA = Visualizer_MarkerAttrPriority[A->Attribute];
	int PrioB = Visualizer_MarkerAttrPriority[B->Attribute];
	return (PrioA > PrioB) - (PrioA < PrioB);
}

rm_handle_t Visualizer_NewMarker(
	rm_handle_t ArrayHandle,
	intptr_t iPosition,
	uint8_t bUpdateValue,
	isort_t NewValue,
	Visualizer_MarkerAttribute Attribute
) {

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);

	tree234** aptreeMarkerMap = pArrayProp->aptreeMarkerMap;

	//

	Visualizer_Marker* Marker = malloc_guarded(sizeof(Visualizer_Marker));
	Marker->ArrayHandle = ArrayHandle;
	Marker->iPosition = iPosition;
	Marker->Attribute = Attribute;

	// Add the new marker (shared pointer between resource table and map)

	rm_handle_t MarkerHandle = AddResource(&Visualizer_MarkerResourceTable, Marker);
	add234(aptreeMarkerMap[iPosition], Marker);

	// Update visually (call renderer)
	
	// Get the highest priority marker
	Visualizer_Marker* HighestPriority = findrel234(aptreeMarkerMap[iPosition], NULL, NULL, REL234_LT);

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;

	Visualizer_RendererEntry.UpdateItem(
		ArrayHandle,
		iPosition,
		UpdateRequest,
		NewValue,
		HighestPriority->Attribute
	);

	return MarkerHandle;

}

void Visualizer_RemoveMarker(
	rm_handle_t ArrayHandle,
	rm_handle_t MarkerHandle
) {
	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	tree234** aptreeMarkerMap = pArrayProp->aptreeMarkerMap;

	// Delete from table & map

	Visualizer_Marker* Marker = RemoveResource(&Visualizer_MarkerResourceTable, MarkerHandle);
	if (!Marker)
		return;
	intptr_t iPosition = Marker->iPosition;
	del234(pArrayProp->aptreeMarkerMap[iPosition], Marker);
	free(Marker);

	// Update visually (call renderer)

	// Get the highest priority marker
	Visualizer_Marker* HighestPriority = findrel234(aptreeMarkerMap[iPosition], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal
	Visualizer_MarkerAttribute TargetAttribute =
		HighestPriority ?
		HighestPriority->Attribute : 
		Visualizer_MarkerAttribute_Normal;

	Visualizer_RendererEntry.UpdateItem(
		ArrayHandle,
		iPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);

	return;
}

// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(rm_handle_t ArrayHandle, intptr_t iPosition, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return;
	if (iPosition >= pArrayProp->Size || iPosition < 0) return;

	intptr_t MarkerHandle = Visualizer_NewMarker(
		ArrayHandle,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveMarker(ArrayHandle, MarkerHandle);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(rm_handle_t ArrayHandle, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return;
	if (iPositionA >= pArrayProp->Size || iPositionA < 0) return;
	if (iPositionB >= pArrayProp->Size || iPositionB < 0) return;

	intptr_t MarkerHandleA = Visualizer_NewMarker(
		ArrayHandle,
		iPositionA,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	intptr_t MarkerHandleB = Visualizer_NewMarker(
		ArrayHandle,
		iPositionB,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveMarker(ArrayHandle, MarkerHandleA);
	Visualizer_RemoveMarker(ArrayHandle, MarkerHandleB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t ArrayHandle, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return;
	if (iPosition >= pArrayProp->Size || iPosition < 0) return;

	intptr_t MarkerHandle = Visualizer_NewMarker(
		ArrayHandle,
		iPosition,
		true,
		NewValue,
		Visualizer_MarkerAttribute_Write
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveMarker(ArrayHandle, MarkerHandle);

	return;

}

void Visualizer_UpdateWrite2(
	intptr_t ArrayHandle,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return;
	if (iPositionA >= pArrayProp->Size || iPositionA < 0) return;
	if (iPositionB >= pArrayProp->Size || iPositionB < 0) return;


	intptr_t MarkerHandleA = Visualizer_NewMarker(
		ArrayHandle,
		iPositionA,
		true,
		NewValueA,
		Visualizer_MarkerAttribute_Write
	);
	intptr_t MarkerHandleB = Visualizer_NewMarker(
		ArrayHandle,
		iPositionB,
		true,
		NewValueB,
		Visualizer_MarkerAttribute_Write
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveMarker(ArrayHandle, MarkerHandleA);
	Visualizer_RemoveMarker(ArrayHandle, MarkerHandleB);

	return;

}

#endif
