
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
	rm_handle_t Handle; // For comparision
	rm_handle_t ArrayHandle;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_Marker;

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	// Initialize Visualizer_ArrayPropResourceTable

	CreateResourceTable(&Visualizer_ArrayPropResourceTable);
	CreateResourceTable(&Visualizer_MarkerResourceTable);

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

static int Visualizer_MarkerPriorityCmp(
	Visualizer_Marker* pMarkerA,
	Visualizer_Marker* pMarkerB
);

rm_handle_t Visualizer_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return INVALID_HANDLE;
	if (Size < 1) return INVALID_HANDLE;
	
	Visualizer_ArrayProp* pArrayProp = malloc_guarded(sizeof(Visualizer_ArrayProp));

	pArrayProp->Size = Size;
	pArrayProp->aptreeMarkerMap = malloc_guarded(Size * sizeof(tree234*));
	for (intptr_t i = 0; i < Size; ++i)
		pArrayProp->aptreeMarkerMap[i] = newtree234(Visualizer_MarkerPriorityCmp);

	rm_handle_t ArrayHandle = AddResource(&Visualizer_ArrayPropResourceTable, pArrayProp);

	// Call Renderer
	Visualizer_RendererEntry.AddArray(ArrayHandle, Size, aArrayState, ValueMin, ValueMax);

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
		// Remove marker from resource table & free memory
		for (
			Visualizer_Marker* pMarker = delpos234(pArrayProp->aptreeMarkerMap[i], 0);
			pMarker != NULL;
			pMarker = delpos234(pArrayProp->aptreeMarkerMap[i], 0)
		) {
			RemoveResource(&Visualizer_MarkerResourceTable, pMarker->Handle);
			free(pMarker);
		}
		freetree234(pArrayProp->aptreeMarkerMap[i]);
	}
	free(pArrayProp->aptreeMarkerMap);

	// Call renderer

	Visualizer_RendererEntry.RemoveArray(ArrayHandle);

	return;

}

void Visualizer_UpdateArray(
	rm_handle_t ArrayHandle,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {

	if (!Visualizer_bInitialized) return;
	if (ValueMax <= ValueMin) return;

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return;

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
				RemoveResource(&Visualizer_MarkerResourceTable, pMarker->Handle);
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
		ArrayHandle,
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
	uintptr_t ValueA = (PrioA == PrioB) ? pMarkerA->Handle : PrioA;
	uintptr_t ValueB = (PrioA == PrioB) ? pMarkerB->Handle : PrioB;
	return (ValueA > ValueB) - (ValueA < ValueB);
}

rm_handle_t Visualizer_NewMarker(
	rm_handle_t ArrayHandle,
	intptr_t iPosition,
	uint8_t bUpdateValue,
	isort_t NewValue,
	Visualizer_MarkerAttribute Attribute
) {

	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, ArrayHandle);
	if (!pArrayProp) return INVALID_HANDLE;
	if (iPosition >= pArrayProp->Size || iPosition < 0) return INVALID_HANDLE;

	Visualizer_Marker* pMarker = malloc_guarded(sizeof(Visualizer_Marker));
	pMarker->ArrayHandle = ArrayHandle;
	pMarker->iPosition = iPosition;
	pMarker->Attribute = Attribute;

	// Add the new marker (pointer is shared between resource table and map)

	rm_handle_t MarkerHandle = AddResource(&Visualizer_MarkerResourceTable, pMarker);
	add234(pArrayProp->aptreeMarkerMap[iPosition], pMarker);

	// Get the highest priority marker

	Visualizer_Marker* pHighestPriorityMarker = findrel234(pArrayProp->aptreeMarkerMap[iPosition], NULL, NULL, REL234_LT);

	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;

	Visualizer_RendererEntry.UpdateItem(
		ArrayHandle,
		iPosition,
		UpdateRequest,
		NewValue,
		pHighestPriorityMarker->Attribute
	);

	return MarkerHandle;

}

void Visualizer_RemoveMarker(rm_handle_t MarkerHandle) {

	// Delete from table & map

	Visualizer_Marker* pMarker = RemoveResource(&Visualizer_MarkerResourceTable, MarkerHandle);
	if (!pMarker)
		return;
	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, pMarker->ArrayHandle);

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
		pMarker->ArrayHandle,
		iPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		TargetAttribute
	);
	free(pMarker);

	return;
}

// On the same array & keep attribute
void Visualizer_MoveMarker(rm_handle_t MarkerHandle, intptr_t iNewPosition) {

	Visualizer_Marker* pMarker = GetResource(&Visualizer_MarkerResourceTable, MarkerHandle);
	if (!pMarker)
		return;
	Visualizer_ArrayProp* pArrayProp = GetResource(&Visualizer_ArrayPropResourceTable, pMarker->ArrayHandle);
	if (iNewPosition >= pArrayProp->Size || iNewPosition < 0) return;

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
		pMarker->ArrayHandle,
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
		pMarker->ArrayHandle,
		iNewPosition,
		AV_RENDERER_UPDATEATTR,
		0,
		pHighestPriorityMarker->Attribute
	);

	return;
}

// Read & Write

void Visualizer_UpdateRead(rm_handle_t ArrayHandle, intptr_t iPosition, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	rm_handle_t MarkerHandle = Visualizer_NewMarker(
		ArrayHandle,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(MarkerHandle);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(rm_handle_t ArrayHandle, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	rm_handle_t MarkerHandleA = Visualizer_NewMarker(
		ArrayHandle,
		iPositionA,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	rm_handle_t MarkerHandleB = Visualizer_NewMarker(
		ArrayHandle,
		iPositionB,
		false,
		0,
		Visualizer_MarkerAttribute_Read
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(MarkerHandleA);
	Visualizer_RemoveMarker(MarkerHandleB);

	return;

}

void Visualizer_UpdateReadMulti(
	rm_handle_t ArrayHandle,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (Length < 0) return;

	rm_handle_t* aMarkerHandle = malloc_guarded(Length * sizeof(rm_handle_t));
	for (intptr_t i = 0; i < Length; ++i) {
		aMarkerHandle[i] = Visualizer_NewMarker(
			ArrayHandle,
			iStartPosition + i,
			false,
			0,
			Visualizer_MarkerAttribute_Read
		);
	}
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = iStartPosition; i < (iStartPosition + Length); ++i)
		Visualizer_RemoveMarker(aMarkerHandle[i]);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(rm_handle_t ArrayHandle, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	rm_handle_t MarkerHandle = Visualizer_NewMarker(
		ArrayHandle,
		iPosition,
		true,
		NewValue,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(MarkerHandle);

	return;

}

void Visualizer_UpdateWrite2(
	rm_handle_t ArrayHandle,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;

	rm_handle_t MarkerHandleA = Visualizer_NewMarker(
		ArrayHandle,
		iPositionA,
		true,
		NewValueA,
		Visualizer_MarkerAttribute_Write
	);
	rm_handle_t MarkerHandleB = Visualizer_NewMarker(
		ArrayHandle,
		iPositionB,
		true,
		NewValueB,
		Visualizer_MarkerAttribute_Write
	);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(MarkerHandleA);
	Visualizer_RemoveMarker(MarkerHandleB);

	return;

}

void Visualizer_UpdateWriteMulti(
	rm_handle_t ArrayHandle,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
) {

	if (!Visualizer_bInitialized) return;
	if (Length < 0) return;

	rm_handle_t* aMarkerHandle = malloc_guarded(Length * sizeof(rm_handle_t));
	for (intptr_t i = 0; i < Length; ++i) {
		aMarkerHandle[i] = Visualizer_NewMarker(
			ArrayHandle,
			iStartPosition + i,
			true,
			aNewValue[i],
			Visualizer_MarkerAttribute_Write
		);
	}
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = iStartPosition; i < (iStartPosition + Length); ++i)
		Visualizer_RemoveMarker(aMarkerHandle[i]);

	return;

}

// Pointer
// TODO: Improve

rm_handle_t Visualizer_CreatePointer(
	rm_handle_t ArrayHandle,
	intptr_t iPosition
) {
	if (!Visualizer_bInitialized) return INVALID_HANDLE;

	return Visualizer_NewMarker(
		ArrayHandle,
		iPosition,
		false,
		0,
		Visualizer_MarkerAttribute_Pointer
	);
}

void Visualizer_RemovePointer(
	rm_handle_t MarkerHandle
) {
	if (!Visualizer_bInitialized) return;

	Visualizer_RemoveMarker(MarkerHandle);

	return;
}

void Visualizer_MovePointer(
	rm_handle_t MarkerHandle,
	intptr_t iNewPosition
) {
	if (!Visualizer_bInitialized) return;

	Visualizer_MoveMarker(MarkerHandle, iNewPosition);

	return;
}

#endif