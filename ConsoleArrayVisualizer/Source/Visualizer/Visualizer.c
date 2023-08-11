
#include <string.h>
#include "Sorts.h"

#include "Visualizer/Visualizer.h"

#include "Visualizer/Renderer/ColumnWindowsConsole.h"
#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

#ifndef VISUALIZER_DISABLED

// TODO: More argument checks

static AV_RENDERER_ENTRY Visualizer_vreRendererEntry;

static const uint64_t Visualizer_TimeDefaultDelay = 10000; // microseconds
static bool Visualizer_bInitialized = false;

static resource_table_t Visualizer_rtArrayProp; // Resource table of AV_ARRAYPROP
static resource_table_t Visualizer_rtUniqueMarker; // Resource table of Visualizer_unique_marker

typedef struct {
	handle_t hArray;
	intptr_t iPos;
	AvAttribute Attribute;
} Visualizer_unique_marker;


void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	// Initialize Visualizer_rtArrayProp

	CreateResourceTable(&Visualizer_rtArrayProp);

	// Only for now
	
	Visualizer_vreRendererEntry.Initialize   = RendererCwc_Initialize;
	Visualizer_vreRendererEntry.Uninitialize = RendererCwc_Uninitialize;

	Visualizer_vreRendererEntry.AddArray     = RendererCwc_AddArray;
	Visualizer_vreRendererEntry.RemoveArray  = RendererCwc_RemoveArray;
	Visualizer_vreRendererEntry.UpdateArray  = RendererCwc_UpdateArray;

	Visualizer_vreRendererEntry.UpdateItem   = RendererCwc_UpdateItem;

	/*
	Visualizer_vreRendererEntry.Initialize   = RendererCvt_Initialize;
	Visualizer_vreRendererEntry.Uninitialize = RendererCvt_Uninitialize;

	Visualizer_vreRendererEntry.AddArray     = RendererCvt_AddArray;
	Visualizer_vreRendererEntry.RemoveArray  = RendererCvt_RemoveArray;
	Visualizer_vreRendererEntry.UpdateArray  = RendererCvt_UpdateArray;

	Visualizer_vreRendererEntry.UpdateItem   = RendererCvt_UpdateItem;

	*/
	// Call renderer

	Visualizer_vreRendererEntry.Initialize();

	Visualizer_bInitialized = true;

	return;

}

void Visualizer_Uninitialize() {

	if (!Visualizer_bInitialized) return;

	Visualizer_bInitialized = false;

	// Uninitialize Visualizer_rtArrayProp

	intptr_t nRemainingArrayProp;
	AV_ARRAYPROP** apRemainingArrayProp = GetResourceList(&Visualizer_rtArrayProp, &nRemainingArrayProp);
	for (intptr_t i = 0; i < nRemainingArrayProp; ++i)
		free(apRemainingArrayProp[i]);
	free(apRemainingArrayProp);

	DestroyResourceTable(&Visualizer_rtArrayProp);

	// Call renderer
	Visualizer_vreRendererEntry.Uninitialize();

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

static int Visualizer_UniqueMarkerPriorityCmp(void* pA, void* pB);

handle_t Visualizer_AddArray(intptr_t Size) {

	if (!Visualizer_bInitialized) return;
	if (Size < 1) return;
	
	//

	AV_ARRAYPROP* pvapArrayProp = malloc_guarded(sizeof(AV_ARRAYPROP));

	pvapArrayProp->Size = Size;
	pvapArrayProp->aptreeUniqueMarkerMap = malloc_guarded(Size * sizeof(tree234*));
	for (intptr_t i = 0; i < Size; ++i)
		pvapArrayProp->aptreeUniqueMarkerMap[i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);

	//
	handle_t hArray = AddResource(&Visualizer_rtArrayProp, pvapArrayProp);

	// Call Renderer
	Visualizer_vreRendererEntry.AddArray(hArray, Size);

	return;

}

void Visualizer_RemoveArray(handle_t hArray) {

	if (!Visualizer_bInitialized) return;

	// Delete from resource table

	AV_ARRAYPROP* pvapArrayProp = RemoveResource(&Visualizer_rtArrayProp, hArray);
	if (!pvapArrayProp) return;
	intptr_t Size = pvapArrayProp->Size;

	// Uninitialize aptreeUniqueMarkerMap

	for (intptr_t i = 0; i < Size; ++i) {
		for (
			void* p = delpos234(pvapArrayProp->aptreeUniqueMarkerMap[i], 0);
			p != NULL;
			p = delpos234(pvapArrayProp->aptreeUniqueMarkerMap[i], 0)
		) {
			free(p);
		}
		freetree234(pvapArrayProp->aptreeUniqueMarkerMap[i]);
	}
	free(pvapArrayProp->aptreeUniqueMarkerMap);

	// Call renderer

	Visualizer_vreRendererEntry.RemoveArray(hArray);

	return;

}

void Visualizer_UpdateArray(
	intptr_t hArray,
	isort_t NewSize,
	isort_t* aNewArrayState,
	int32_t bVisible,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return;
	if (ValueMax <= ValueMin) return;

	AV_ARRAYPROP* pvapArrayProp = GetResource(&Visualizer_rtArrayProp, hArray);
	if (!pvapArrayProp) return;

	// Handle array resize

	if ((NewSize > 0) && (NewSize != pvapArrayProp->Size)) {

		// Resize the marker map
		// TODO: MEMORY LEAK

		tree234** aptreeResizedUmMap = realloc_guarded(pvapArrayProp->aptreeUniqueMarkerMap, NewSize);
		intptr_t OldSize = pvapArrayProp->Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Initialize the new part

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aptreeResizedUmMap[OldSize + i] = newtree234(Visualizer_UniqueMarkerPriorityCmp);
		pvapArrayProp->aptreeUniqueMarkerMap = aptreeResizedUmMap;
		pvapArrayProp->Size = NewSize;

	}

	// Call renderer

	Visualizer_vreRendererEntry.UpdateArray(
		hArray,
		NewSize,
		aNewArrayState,
		bVisible,
		ValueMin,
		ValueMax
	);

	return;
}

// Marker

static const int Visualizer_UniqueMarkerAttrPriority[] = {
	0, //AvAttribute_Background
	1, //AvAttribute_Normal
	3, //AvAttribute_Read
	4, //AvAttribute_Write
	2, //AvAttribute_Pointer
	5, //AvAttribute_Correct
	6, //AvAttribute_Incorrect
};

static int Visualizer_UniqueMarkerPriorityCmp(void* pA, void* pB) {
	// Compare by attribute
	Visualizer_unique_marker* pumA = pA;
	Visualizer_unique_marker* pumB = pB;
	int PrioA = Visualizer_UniqueMarkerAttrPriority[pumA->Attribute];
	int PrioB = Visualizer_UniqueMarkerAttrPriority[pumB->Attribute];
	return (PrioA > PrioB) - (PrioA < PrioB);
}

handle_t Visualizer_NewUniqueMarker(
	handle_t hArray,
	intptr_t iPos,
	uint8_t bUpdateValue,
	isort_t NewValue,
	AvAttribute Attribute
) {

	AV_ARRAYPROP* pvapArrayProp = GetResource(&Visualizer_rtArrayProp, hArray);

	tree234** aptreeUniqueMarkerMap = pvapArrayProp->aptreeUniqueMarkerMap;

	// Acquire new ID (unique)

	Visualizer_unique_marker* pumUniqueMarker = malloc(sizeof(Visualizer_unique_marker));
	pumUniqueMarker->hArray = hArray;
	pumUniqueMarker->iPos = iPos;
	pumUniqueMarker->Attribute = Attribute;
	handle_t hMarker = AddResource(&Visualizer_rtUniqueMarker, pumUniqueMarker);

	// Add the new marker to the map

	add234(aptreeUniqueMarkerMap[iPos], );

	// Update visually (call renderer)
	
	// Get the highest priority marker
	Visualizer_unique_marker* pumHighestPriority = findrel234(aptreeUniqueMarkerMap[iPos], NULL, NULL, REL234_LT);

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;

	Visualizer_vreRendererEntry.UpdateItem(
		hArray,
		iPos,
		UpdateRequest,
		NewValue,
		pumHighestPriority->Attribute
	);

	return hMarker;

}

void Visualizer_RemoveUniqueMarker(
	handle_t hArray,
	handle_t hMarker
) {
	AV_ARRAYPROP* pvapArrayProp = GetResource(&Visualizer_rtArrayProp, hArray);
	tree234** aptreeUniqueMarkerMap = pvapArrayProp->aptreeUniqueMarkerMap;

	// Save iPos

	Visualizer_unique_marker* pumUniqueMarker = GetResource(&Visualizer_rtUniqueMarker, hMarker);
	if (!pumUniqueMarker)
		return;
	intptr_t iPos = pumUniqueMarker->iPos;

	// Delete from table

	RemoveResource(&Visualizer_rtUniqueMarker, hMarker);
	del234(pvapArrayProp->aptreeUniqueMarkerMap, );
	free(pumUniqueMarker);

	// Update visually (call renderer)

	// Get the highest priority marker
	Visualizer_unique_marker* pumHighestPriority = findrel234(aptreeUniqueMarkerMap[iPos], NULL, NULL, REL234_LT);

	// If 0 marker are in the map slot, reset to normal
	AvAttribute TargetAttribute = AvAttribute_Normal;
	if (pumHighestPriority)
		TargetAttribute = pumHighestPriority->Attribute;

	Visualizer_vreRendererEntry.UpdateItem(
		hArray,
		iPos,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);

	return 0;
}

// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(handle_t hArray, intptr_t iPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	AV_ARRAYPROP* pvapArrayProp = GetResource(&Visualizer_rtArrayProp, hArray);
	if (!pvapArrayProp) return;
	if (iPos >= pvapArrayProp->Size || iPos < 0) return;

	intptr_t MarkerId = Visualizer_NewUniqueMarker(
		hArray,
		iPos,
		false,
		0,
		AvAttribute_Read
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(hArray, MarkerId);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(handle_t hArray, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	AV_ARRAYPROP* pvapArrayProp = GetResource(&Visualizer_rtArrayProp, hArray);
	if (!pvapArrayProp) return;
	if (iPosA >= pvapArrayProp->Size || iPosA < 0) return;
	if (iPosB >= pvapArrayProp->Size || iPosB < 0) return;

	intptr_t MarkerIdA = Visualizer_NewUniqueMarker(
		hArray,
		iPosA,
		false,
		0,
		AvAttribute_Read
	);
	intptr_t MarkerIdB = Visualizer_NewUniqueMarker(
		hArray,
		iPosB,
		false,
		0,
		AvAttribute_Read
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(hArray, MarkerIdA);
	Visualizer_RemoveUniqueMarker(hArray, MarkerIdB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t hArray, intptr_t iPos, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	AV_ARRAYPROP* pvapArrayProp = GetResource(&Visualizer_rtArrayProp, hArray);
	if (!pvapArrayProp) return;
	if (iPos >= pvapArrayProp->Size || iPos < 0) return;

	intptr_t MarkerId = Visualizer_NewUniqueMarker(
		hArray,
		iPos,
		true,
		NewValue,
		AvAttribute_Write
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(hArray, MarkerId);

	return;

}

void Visualizer_UpdateWrite2(
	intptr_t hArray,
	intptr_t iPosA,
	intptr_t iPosB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;

	AV_ARRAYPROP* pvapArrayProp = GetResource(&Visualizer_rtArrayProp, hArray);
	if (!pvapArrayProp) return;
	if (iPosA >= pvapArrayProp->Size || iPosA < 0) return;
	if (iPosB >= pvapArrayProp->Size || iPosB < 0) return;


	intptr_t MarkerIdA = Visualizer_NewUniqueMarker(
		hArray,
		iPosA,
		true,
		NewValueA,
		AvAttribute_Write
	);
	intptr_t MarkerIdB = Visualizer_NewUniqueMarker(
		hArray,
		iPosB,
		true,
		NewValueB,
		AvAttribute_Write
	);

	//
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_RemoveUniqueMarker(hArray, MarkerIdA);
	Visualizer_RemoveUniqueMarker(hArray, MarkerIdB);

	return;

}

#endif
