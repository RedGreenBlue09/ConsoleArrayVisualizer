
#include <stdlib.h>
#include "Sorts.h"
#include "Visualizer.h"
#include "Utils.h"

static const uint64_t Visualizer_TimeDefaultDelay = 100; // miliseconds
static uint8_t Visualizer_bInitialized = FALSE;

// Todo: tree struct
static AV_ARRAY Visualizer_aVArrayList[AV_MAX_ARRAY_COUNT];

// Init

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	// Initialize Renderer
	RendererWcc_Initialize();

	for (intptr_t i = 0; i < AV_MAX_ARRAY_COUNT; ++i) {

		Visualizer_aVArrayList[i].bActive = FALSE;

		Visualizer_aVArrayList[i].Size = 0;
		Visualizer_aVArrayList[i].aArrayState = NULL;
		Visualizer_aVArrayList[i].aAttribute = NULL;

		Visualizer_aVArrayList[i].bVisible = FALSE;
		Visualizer_aVArrayList[i].ValueMin = 0;
		Visualizer_aVArrayList[i].ValueMax = 0;

		Visualizer_aVArrayList[i].nPointer = 0;
		Visualizer_aVArrayList[i].aPointer = NULL;

	}

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

void Visualizer_AddArray(intptr_t ArrayId, intptr_t Size) {

	if (!Visualizer_bInitialized) return;
	if (Visualizer_aVArrayList[ArrayId].bActive) return;

	Visualizer_aVArrayList[ArrayId].bActive = TRUE;

	Visualizer_aVArrayList[ArrayId].Size = Size;
	Visualizer_aVArrayList[ArrayId].aArrayState = malloc(Size * sizeof(isort_t));
	Visualizer_aVArrayList[ArrayId].aAttribute = malloc(Size * sizeof(AvAttribute));

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 1;

	Visualizer_aVArrayList[ArrayId].nPointer = AV_MAX_POINTER_COUNT;
	Visualizer_aVArrayList[ArrayId].aPointer = malloc(AV_MAX_POINTER_COUNT * sizeof(intptr_t));

	if (!Visualizer_aVArrayList[ArrayId].aArrayState)
		abort();
	if (!Visualizer_aVArrayList[ArrayId].aAttribute)
		abort();
	if (!Visualizer_aVArrayList[ArrayId].aPointer)
		abort();// TODO: Whole program: force abort on error

	// Set array state items to 0
	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aVArrayList[ArrayId].aArrayState[i] = 0;

	// Set all attributes to AR_ATTR_NORMAL
	for (intptr_t i = 0; i < Size; ++i)
		Visualizer_aVArrayList[ArrayId].aAttribute[i] = AvAttribute_Normal;

	// Set all pointers to -1
	for (intptr_t i = 0; i < AV_MAX_POINTER_COUNT; ++i)
		Visualizer_aVArrayList[ArrayId].aPointer[i] = (-1);

	RendererWcc_AddArray(ArrayId, &Visualizer_aVArrayList[ArrayId]);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;

	//
	Visualizer_aVArrayList[ArrayId].bActive = FALSE;

	Visualizer_aVArrayList[ArrayId].Size = 0;
	Visualizer_aVArrayList[ArrayId].aArrayState = NULL;
	free(Visualizer_aVArrayList[ArrayId].aAttribute);
	Visualizer_aVArrayList[ArrayId].aAttribute = NULL;

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 0;

	Visualizer_aVArrayList[ArrayId].nPointer = 0;
	free(Visualizer_aVArrayList[ArrayId].aPointer);
	Visualizer_aVArrayList[ArrayId].aPointer = NULL;

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
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

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
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size) return;

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
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

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
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size) return;

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


static uint8_t Visualizer_IsPointerOverlapped(intptr_t ArrayId, intptr_t PointerId) {

	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	uint8_t isOverlapping = FALSE;
	for (intptr_t i = 0; i < nPointer; ++i) {
		if (
			(i != PointerId) &&
			(aPointer[i] == aPointer[PointerId])
			) {
			isOverlapping = TRUE;
			break;
		}
	}

	return isOverlapping;

}

void Visualizer_UpdatePointer(intptr_t ArrayId, intptr_t PointerId, intptr_t iNewPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (iNewPos >= Size) return;
	if (PointerId >= nPointer) return;

	if (
		(aPointer[PointerId] != (-1)) &&
		(!Visualizer_IsPointerOverlapped(ArrayId, PointerId))
		) {

		// Reset old pointer to normal.
		Visualizer_aVArrayList[ArrayId].aAttribute[aPointer[PointerId]] = AvAttribute_Normal;

		RendererWcc_DrawItem(ArrayId, aPointer[PointerId], aArrayState[aPointer[PointerId]], AvAttribute_Normal);

	}

	Visualizer_aVArrayList[ArrayId].aAttribute[iNewPos] = AvAttribute_Pointer;

	RendererWcc_DrawItem(ArrayId, iNewPos, aArrayState[iNewPos], AvAttribute_Pointer);

	Visualizer_Sleep(fSleepMultiplier);
	aPointer[PointerId] = iNewPos;

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArrayState = Visualizer_aVArrayList[ArrayId].aArrayState;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (PointerId >= nPointer) return;

	if (
		(aPointer[PointerId] != (-1)) &&
		(!Visualizer_IsPointerOverlapped(ArrayId, PointerId))
		) {

		// Reset old pointer to normal.
		Visualizer_aVArrayList[ArrayId].aAttribute[aPointer[PointerId]] = AvAttribute_Normal;

		RendererWcc_DrawItem(ArrayId, aPointer[PointerId], aArrayState[aPointer[PointerId]], AvAttribute_Normal);

	}

	aPointer[PointerId] = (intptr_t)(-1);

	return;

}
