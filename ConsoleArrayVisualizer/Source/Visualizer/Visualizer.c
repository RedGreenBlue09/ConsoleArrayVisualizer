
#include <stdlib.h>
#include "Sorts.h"
#include "Visualizer.h"

#include "Utils.h"
#include "GuardedMalloc.h"

#ifndef VISUALIZER_DISABLED

// TODO: Overlapped marker system with priority queue
//       Write & swap functions will change the value before sleep

static const uint64_t Visualizer_TimeDefaultDelay = 10000; // microseconds
static uint8_t Visualizer_bInitialized = FALSE;

// TODO: Maybe tree struct for this.
static AV_ARRAYPROP Visualizer_aArrayProp[AV_MAX_ARRAY_COUNT];

// Init

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	RendererWcc_Initialize();

	memset(Visualizer_aArrayProp, 0, sizeof(Visualizer_aArrayProp));

	Visualizer_bInitialized = TRUE;
	return;

}

void Visualizer_Uninitialize() {

	if (!Visualizer_bInitialized) return;

	RendererWcc_Uninitialize();
	Visualizer_bInitialized = FALSE;

	return;
}

// Sleep

void Visualizer_Sleep(double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	sleep64((uint64_t)((double)Visualizer_TimeDefaultDelay * fSleepMultiplier));
	return;

}

// Array

int VpidCompare(void* pA, void* pB);

void Visualizer_AddArray(intptr_t ArrayId, intptr_t Size) {

	if (!Visualizer_bInitialized) return;
	if (Visualizer_aArrayProp[ArrayId].bActive) return;
	if (Size < 1) return;

	Visualizer_aArrayProp[ArrayId].bActive = TRUE;
	Visualizer_aArrayProp[ArrayId].Size = Size;
	Visualizer_aArrayProp[ArrayId].aPointerCount = malloc_guarded(Size * sizeof(intptr_t));

	// Initialize arrays

	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aArrayProp[ArrayId].aPointerCount[i] = 0;

	Visualizer_aArrayProp[ArrayId].ptreePointerId = newtree234(VpidCompare);

	// Call visualizer

	RendererWcc_AddArray(ArrayId, Size);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;

	free(Visualizer_aArrayProp[ArrayId].aPointerCount);
	freetree234(Visualizer_aArrayProp[ArrayId].ptreePointerId);

	memset(&Visualizer_aArrayProp[ArrayId], 0, sizeof(Visualizer_aArrayProp[ArrayId]));

	RendererWcc_RemoveArray(ArrayId);

	return;

}

void Visualizer_UpdateArray(intptr_t ArrayId, isort_t NewSize, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	// Handle array resize

	if ((NewSize > 0) && (NewSize != Visualizer_aArrayProp[ArrayId].Size)) {
		// TODO: Resize pointer count array

		isort_t* aResizedArrayState = realloc_guarded(Visualizer_aArrayProp[ArrayId].aArrayState, NewSize);

		intptr_t OldSize = Visualizer_aArrayProp[ArrayId].Size;
		intptr_t NewPartSize = NewSize - OldSize;

		// Fill the new part with 0

		for (intptr_t i = 0; i < NewPartSize; ++i)
			aResizedArrayState[OldSize + i] = 0;

		Visualizer_aArrayProp[ArrayId].Size = NewSize;
		Visualizer_aArrayProp[ArrayId].aArrayState = aResizedArrayState;


	}

	isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;
	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;

	// Handle new array state

	if (aNewArrayState)
		for (intptr_t i = 0; i < Size; ++i)
			aArrayState[i] = aNewArrayState[i];

	// Call renderer

	RendererWcc_UpdateArray(ArrayId, NewSize, aNewArrayState, bVisible, ValueMin, ValueMax);

	return;
}

// Marker

typedef struct {
	intptr_t UniqueId;
	intptr_t iPos;
	AvAttribute Attribute;
} AV_UNIQUEMARKER;

intptr_t Visualizer_NewUniqueMarker(
	intptr_t ArrayId,
	intptr_t iPos,
	uint8_t bUpdateValue,
	isort_t NewValue,
	AvAttribute Attribute
) {

	tree234* ptreeUniqueMarker = Visualizer_aArrayProp[ArrayId].ptreeUniqueMarker;
	DYNAMIC_ARRAY* pdaUniqueMarkerIdHoles = &Visualizer_aArrayProp[ArrayId].daUniqueMarkerIdHoles;
	tree234** aptreeUniqueMarkerMap = Visualizer_aArrayProp[ArrayId].aptreeUniqueMarkerMap;

	// Acquire new ID (unique)
	if (DaSize(pdaUniqueMarkerIdHoles)) // Out of ids
		return INTPTR_MIN; // INTPTR_MIN is reserved for error handling
	intptr_t* pNewId = DaIndex(pdaUniqueMarkerIdHoles, 0);

	// Add new unique marker to id tree
	AV_UNIQUEMARKER* pvumNewUniqueMarker = malloc_guarded(sizeof(AV_UNIQUEMARKER));
	pvumNewUniqueMarker->UniqueId = *pNewId;
	pvumNewUniqueMarker->iPos = iPos;
	add234(ptreeUniqueMarker, pvumNewUniqueMarker);

	// Add the new marker iPos slot
	addpos234(aptreeUniqueMarkerMap[iPos], pvumNewUniqueMarker, 0);

	// Call renderer
	// The newly added marker is the highest priority one
	uint32_t UpdateRequest = AV_RENDERER_UPDATEATTR;
	if (bUpdateValue)
		UpdateRequest |= AV_RENDERER_UPDATEVALUE;

	RendererWcc_UpdateItem(
		ArrayId,
		iPos,
		UpdateRequest,
		NewValue,
		Attribute
	);

	return *pNewId;

}


// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPos >= Visualizer_aArrayProp[ArrayId].Size || iPos < 0) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;

	AvAttribute AttrOld = Visualizer_aArrayProp[ArrayId].aAttribute[iPos];

	RendererWcc_UpdateItem(ArrayId, iPos, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_UpdateItem(ArrayId, iPos, AV_RENDERER_UPDATEATTR, 0, AttrOld);

	return;

}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aArrayProp[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aArrayProp[ArrayId].Size || iPosB < 0) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;

	AvAttribute AttrOldA = Visualizer_aArrayProp[ArrayId].aAttribute[iPosA];
	AvAttribute AttrOldB = Visualizer_aArrayProp[ArrayId].aAttribute[iPosB];

	RendererWcc_UpdateItem(ArrayId, iPosA, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Read);
	RendererWcc_UpdateItem(ArrayId, iPosB, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_UpdateItem(ArrayId, iPosA, AV_RENDERER_UPDATEATTR, 0, AttrOldA);
	RendererWcc_UpdateItem(ArrayId, iPosB, AV_RENDERER_UPDATEATTR, 0, AttrOldB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPos >= Visualizer_aArrayProp[ArrayId].Size || iPos < 0) return;

	isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;
	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;

	uint8_t AttrOld = Visualizer_aArrayProp[ArrayId].aAttribute[iPos];

	RendererWcc_UpdateItem(ArrayId, iPos, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Write);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_UpdateItem(ArrayId, iPos, AV_RENDERER_UPDATEATTR | AV_RENDERER_UPDATEVALUE, NewValue, AttrOld);

	aArrayState[iPos] = NewValue;

	return;

}

void Visualizer_UpdateSwap(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aArrayProp[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aArrayProp[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aArrayProp[ArrayId].Size || iPosB < 0) return;

	isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;

	AvAttribute AttrOldA = Visualizer_aArrayProp[ArrayId].aAttribute[iPosA];
	AvAttribute AttrOldB = Visualizer_aArrayProp[ArrayId].aAttribute[iPosB];

	// Swap the values
	RendererWcc_UpdateItem(ArrayId, iPosA, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Write);
	RendererWcc_UpdateItem(ArrayId, iPosB, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Write);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_UpdateItem(ArrayId, iPosA, AV_RENDERER_UPDATEATTR | AV_RENDERER_UPDATEVALUE, aArrayState[iPosB], AttrOldA);
	RendererWcc_UpdateItem(ArrayId, iPosB, AV_RENDERER_UPDATEATTR | AV_RENDERER_UPDATEVALUE, aArrayState[iPosA], AttrOldB);

	ISORT_SWAP(aArrayState[iPosA], aArrayState[iPosB]);

	return;

}

// Pointer:
// Highlight an item and keep it highlighted until the caller removes the highlight.

typedef struct {
	intptr_t PointerId;
	intptr_t iPos; // The array index that it's pointing to.
} AV_POINTERID;

int VpidCompare(void* pA, void* pB) {

	intptr_t PointerIdA = ((AV_POINTERID*)pA)->PointerId;
	intptr_t PointerIdB = ((AV_POINTERID*)pB)->PointerId;

	return (PointerIdA > PointerIdB) - (PointerIdA < PointerIdB);

};

void Visualizer_UpdatePointer(intptr_t ArrayId, intptr_t PointerId, intptr_t iNewPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;
	//isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;

	if (iNewPos >= Size || iNewPos < 0) return;

	tree234* ptreePointerId = Visualizer_aArrayProp[ArrayId].ptreePointerId;
	intptr_t* aPointerCount = Visualizer_aArrayProp[ArrayId].aPointerCount;

	// Check if pointer is already exist.

	AV_POINTERID vpidCompare = { PointerId, 0 };
	AV_POINTERID* pvpid = find234(ptreePointerId, &vpidCompare, NULL);

	// NULL means isn't exist
	if (pvpid) {

		intptr_t iOldPos = pvpid->iPos;

		// If old pointer don't overlap
		if (aPointerCount[iOldPos] <= 1) {

			// Reset old pointer to normal.

			Visualizer_aArrayProp[ArrayId].aAttribute[iOldPos] = AvAttribute_Normal;

			RendererWcc_UpdateItem(ArrayId, iOldPos, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Normal);

		}

		// Update position

		aPointerCount[iOldPos] -= 1;

	} else {

		// Add new pointer

		pvpid = malloc_guarded(sizeof(AV_POINTERID));

		pvpid->PointerId = PointerId;
		pvpid->iPos = iNewPos;
		add234(ptreePointerId, pvpid);

	}

	// Draw item with Pointer attribute.

	Visualizer_aArrayProp[ArrayId].aAttribute[iNewPos] = AvAttribute_Pointer;
	RendererWcc_UpdateItem(ArrayId, iNewPos, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Pointer);

	Visualizer_Sleep(fSleepMultiplier);

	// Update position.

	aPointerCount[iNewPos] += 1;
	pvpid->iPos = iNewPos;

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aArrayProp[ArrayId].Size;
	//isort_t* aArrayState = Visualizer_aArrayProp[ArrayId].aArrayState;

	tree234* ptreePointerId = Visualizer_aArrayProp[ArrayId].ptreePointerId;
	intptr_t* aPointerCount = Visualizer_aArrayProp[ArrayId].aPointerCount;

	// Check if pointer is already exist.

	AV_POINTERID vpidCompare = { PointerId, 0 };
	AV_POINTERID* pvpid = find234(ptreePointerId, &vpidCompare, NULL);

	if (pvpid) {

		intptr_t iPos = pvpid->iPos;

		// If pointer don't overlap
		if (aPointerCount[iPos] <= 1) {

			// Reset pointer to normal.

			Visualizer_aArrayProp[ArrayId].aAttribute[iPos] = AvAttribute_Normal;

			RendererWcc_UpdateItem(ArrayId, iPos, AV_RENDERER_UPDATEATTR, 0, AvAttribute_Normal);

			// Update position

			aPointerCount[iPos] -= 1;

		}

		// Delete the pointer from memory

		del234(ptreePointerId, pvpid);
		free(pvpid);

	}

	return;

}

#endif
