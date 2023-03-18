
#include <stdlib.h>
#include "Sorts.h"
#include "Visualizer.h"

#include "Utils.h"

static const uint64_t Visualizer_TimeDefaultDelay = 100; // miliseconds
static uint8_t Visualizer_bInitialized = FALSE;

// TODO: Maybe tree struct for this.
static AV_ARRAY Visualizer_aVArrayList[AV_MAX_ARRAY_COUNT];

// Init

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	RendererWcc_Initialize();

	memset(Visualizer_aVArrayList, 0, sizeof(Visualizer_aVArrayList));

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
	if (Visualizer_aVArrayList[ArrayId].bActive) return;
	if (Size < 1) return;

	Visualizer_aVArrayList[ArrayId].bActive = TRUE;

	Visualizer_aVArrayList[ArrayId].Size = Size;
	Visualizer_aVArrayList[ArrayId].aArrayState = malloc(Size * sizeof(isort_t));
	Visualizer_aVArrayList[ArrayId].aAttribute = malloc(Size * sizeof(AvAttribute));

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 1;

	Visualizer_aVArrayList[ArrayId].ptreePointerId = newtree234(VpidCompare);
	Visualizer_aVArrayList[ArrayId].aPointerCount = malloc(Size * sizeof(intptr_t));

	if (!Visualizer_aVArrayList[ArrayId].aArrayState)
		abort();
	if (!Visualizer_aVArrayList[ArrayId].aAttribute)
		abort();
	if (!Visualizer_aVArrayList[ArrayId].aPointerCount)
		abort();// TODO: Whole program: force abort on error

	// Initialize arrays

	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aVArrayList[ArrayId].aArrayState[i] = 0;

	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aVArrayList[ArrayId].aAttribute[i] = AvAttribute_Normal;

	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aVArrayList[ArrayId].aPointerCount[i] = 0;

	RendererWcc_AddArray(ArrayId, &Visualizer_aVArrayList[ArrayId]);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;

	free(Visualizer_aVArrayList[ArrayId].aAttribute);
	free(Visualizer_aVArrayList[ArrayId].aArrayState);
	free(Visualizer_aVArrayList[ArrayId].aPointerCount);
	freetree234(Visualizer_aVArrayList[ArrayId].ptreePointerId);

	memset(&Visualizer_aVArrayList[ArrayId], 0, sizeof(Visualizer_aVArrayList[ArrayId]));

	RendererWcc_RemoveArray(ArrayId);

	return;

}

void Visualizer_UpdateArray(intptr_t ArrayId, isort_t* aNewArrayState, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;

	Visualizer_aVArrayList[ArrayId].bVisible = bVisible;
	Visualizer_aVArrayList[ArrayId].ValueMin = ValueMin;
	Visualizer_aVArrayList[ArrayId].ValueMax = ValueMax;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	if (aNewArrayState) {
		for (intptr_t i = 0; i < Size; ++i)
			aArrayState[i] = aNewArrayState[i];
	}

	RendererWcc_UpdateArray(ArrayId, aNewArrayState, bVisible, ValueMin, ValueMax);

	return;
}

// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size || iPos < 0) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	AvAttribute AttrOld = Visualizer_aVArrayList[ArrayId].aAttribute[iPos];

	RendererWcc_DrawItem(ArrayId, iPos, aArrayState[iPos], AvAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPos, aArrayState[iPos], AttrOld);

	return;

}

// Update 2 items with a single sleep (used for comparisions).
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size || iPosB < 0) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	AvAttribute AttrOldA = Visualizer_aVArrayList[ArrayId].aAttribute[iPosA];
	AvAttribute AttrOldB = Visualizer_aVArrayList[ArrayId].aAttribute[iPosB];

	RendererWcc_DrawItem(ArrayId, iPosA, aArrayState[iPosA], AvAttribute_Read);
	RendererWcc_DrawItem(ArrayId, iPosB, aArrayState[iPosB], AvAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPosA, aArrayState[iPosA], AttrOldA);
	RendererWcc_DrawItem(ArrayId, iPosB, aArrayState[iPosB], AttrOldB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t NewValue, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size || iPos < 0) return;

	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOld = Visualizer_aVArrayList[ArrayId].aAttribute[iPos];

	RendererWcc_DrawItem(ArrayId, iPos, aArrayState[iPos], AvAttribute_Write);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPos, NewValue, AttrOld);

	aArrayState[iPos] = NewValue;

	return;

}

void Visualizer_UpdateSwap(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size || iPosA < 0) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size || iPosB < 0) return;

	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	AvAttribute AttrOldA = Visualizer_aVArrayList[ArrayId].aAttribute[iPosA];
	AvAttribute AttrOldB = Visualizer_aVArrayList[ArrayId].aAttribute[iPosB];

	// Swap the values
	RendererWcc_DrawItem(ArrayId, iPosA, aArrayState[iPosA], AvAttribute_Write);
	RendererWcc_DrawItem(ArrayId, iPosB, aArrayState[iPosB], AvAttribute_Write);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPosA, aArrayState[iPosB], AttrOldA);
	RendererWcc_DrawItem(ArrayId, iPosB, aArrayState[iPosA], AttrOldB);

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

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	if (iNewPos >= Size || iNewPos < 0) return;

	tree234* ptreePointerId = Visualizer_aVArrayList[ArrayId].ptreePointerId;
	intptr_t* aPointerCount = Visualizer_aVArrayList[ArrayId].aPointerCount;

	// Check if pointer is already exist.

	AV_POINTERID vpidCompare = { PointerId, 0 };
	AV_POINTERID* pvpid = find234(ptreePointerId, &vpidCompare, NULL);

	// NULL means isn't exist
	if (pvpid) {

		intptr_t iOldPos = pvpid->iPos;

		// If old pointer don't overlap
		if (aPointerCount[iOldPos] <= 1) {

			// Reset old pointer to normal.

			Visualizer_aVArrayList[ArrayId].aAttribute[iOldPos] = AvAttribute_Normal;

			RendererWcc_DrawItem(ArrayId, iOldPos, aArrayState[iOldPos], AvAttribute_Normal);

		}

		// Update position

		aPointerCount[iOldPos] -= 1;

	} else {

		// Add new pointer

		pvpid = malloc(sizeof(AV_POINTERID)); // TODO: Guarded malloc macro
		if (!pvpid)
			abort();

		pvpid->PointerId = PointerId;
		pvpid->iPos = iNewPos;
		add234(ptreePointerId, pvpid);

	}

	// Draw item with Pointer attribute.

	Visualizer_aVArrayList[ArrayId].aAttribute[iNewPos] = AvAttribute_Pointer;
	RendererWcc_DrawItem(ArrayId, iNewPos, aArrayState[iNewPos], AvAttribute_Pointer);

	Visualizer_Sleep(fSleepMultiplier);

	// Update position.

	aPointerCount[iNewPos] += 1;
	pvpid->iPos = iNewPos;

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;

	tree234* ptreePointerId = Visualizer_aVArrayList[ArrayId].ptreePointerId;
	intptr_t* aPointerCount = Visualizer_aVArrayList[ArrayId].aPointerCount;

	// Check if pointer is already exist.

	AV_POINTERID vpidCompare = { PointerId, 0 };
	AV_POINTERID* pvpid = find234(ptreePointerId, &vpidCompare, NULL);

	if (pvpid) {

		intptr_t iPos = pvpid->iPos;

		// If pointer don't overlap
		if (aPointerCount[iPos] <= 1) {

			// Reset pointer to normal.

			Visualizer_aVArrayList[ArrayId].aAttribute[iPos] = AvAttribute_Normal;

			RendererWcc_DrawItem(ArrayId, iPos, aArrayState[iPos], AvAttribute_Normal);

			// Update position

			aPointerCount[iPos] -= 1;

		}

		// Delete the pointer from memory

		del234(ptreePointerId, pvpid);
		free(pvpid);

	}

	return;

}
