
#include "Visualizer.h"

#ifndef VISUALIZER_DISABLED

static const uint64_t Visualizer_TimeDefaultDelay = 4000;
static uint8_t Visualizer_bInitialized = FALSE;

//
static V_ARRAY Visualizer_aVArrayList[AR_MAX_ARRAY_COUNT];

// Low level renderer functions

void Visualizer_UpdateItem(intptr_t ArrayId, intptr_t iPos, isort_t Value, uint8_t Attr) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

	RendererWcc_DrawItem(ArrayId, iPos, Value, Attr);

	return;
}

// Init

void Visualizer_Initialize() {

	if (Visualizer_bInitialized) return;

	// Initialize Renderer
	RendererWcc_Initialize();

	for (intptr_t i = 0; i < AR_MAX_ARRAY_COUNT; ++i) {

		Visualizer_aVArrayList[i].bActive = FALSE;

		Visualizer_aVArrayList[i].Size = 0;
		Visualizer_aVArrayList[i].aArray = NULL;
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

void Visualizer_AddArray(intptr_t ArrayId, isort_t* aArray, intptr_t Size) {

	if (!Visualizer_bInitialized) return;
	if (Visualizer_aVArrayList[ArrayId].bActive) return;

	Visualizer_aVArrayList[ArrayId].bActive = TRUE;

	Visualizer_aVArrayList[ArrayId].Size = Size;
	Visualizer_aVArrayList[ArrayId].aArray = aArray;
	Visualizer_aVArrayList[ArrayId].aAttribute = malloc(Size * sizeof(intptr_t));

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 0;

	Visualizer_aVArrayList[ArrayId].nPointer = AR_MAX_POINTER_COUNT;
	Visualizer_aVArrayList[ArrayId].aPointer = malloc(AR_MAX_POINTER_COUNT * sizeof(intptr_t));

	for (intptr_t i = 0; i < AR_MAX_POINTER_COUNT; ++i)
		Visualizer_aVArrayList[ArrayId].aPointer[i] = (-1);

	RendererWcc_AddArray(&Visualizer_aVArrayList[ArrayId], ArrayId);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;

	RendererWcc_RemoveArray(ArrayId);

	//
	Visualizer_aVArrayList[ArrayId].bActive = FALSE;

	Visualizer_aVArrayList[ArrayId].Size = 0;
	Visualizer_aVArrayList[ArrayId].aArray = NULL;
	free(Visualizer_aVArrayList[ArrayId].aAttribute);
	Visualizer_aVArrayList[ArrayId].aAttribute = NULL;

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 0;

	Visualizer_aVArrayList[ArrayId].nPointer = AR_MAX_POINTER_COUNT;
	free(Visualizer_aVArrayList[ArrayId].aPointer);
	Visualizer_aVArrayList[ArrayId].aPointer = NULL;


	// This also set all .active to FALSE
	return;

}

void Visualizer_UpdateArray(intptr_t ArrayId, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;

	Visualizer_aVArrayList[ArrayId].bVisible = bVisible;
	Visualizer_aVArrayList[ArrayId].ValueMin = ValueMin;
	Visualizer_aVArrayList[ArrayId].ValueMax = ValueMax;

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	// TODO: Specific renderer function
	for (intptr_t i = 0; i < Size; ++i) {

		RendererWcc_DrawItem(ArrayId, i, aArray[i], AR_ATTR_NORMAL);
	}

	return;
}

// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOld = Visualizer_aVArrayList[ArrayId].aAttribute[iPos];

	RendererWcc_DrawItem(ArrayId, iPos, aArray[iPos], AR_ATTR_READ);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPos, aArray[iPos], AttrOld);

	return;

}

// Update 2 items with a single sleep (used for comparisions).
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size) return;

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOldA = Visualizer_aVArrayList[ArrayId].aAttribute[iPosA];
	uint8_t AttrOldB = Visualizer_aVArrayList[ArrayId].aAttribute[iPosB];

	RendererWcc_DrawItem(ArrayId, iPosA, aArray[iPosA], AR_ATTR_READ);
	RendererWcc_DrawItem(ArrayId, iPosB, aArray[iPosB], AR_ATTR_READ);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPosA, aArray[iPosA], AttrOldA);
	RendererWcc_DrawItem(ArrayId, iPosB, aArray[iPosB], AttrOldB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t Value, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPos >= Visualizer_aVArrayList[ArrayId].Size) return;

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOld = Visualizer_aVArrayList[ArrayId].aAttribute[iPos];

	RendererWcc_DrawItem(ArrayId, iPos, aArray[iPos], AR_ATTR_WRITE);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPos, Value, AttrOld);

	return;

}

void Visualizer_UpdateSwap(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;
	if (!Visualizer_aVArrayList[ArrayId].bActive) return;
	if (iPosA >= Visualizer_aVArrayList[ArrayId].Size) return;
	if (iPosB >= Visualizer_aVArrayList[ArrayId].Size) return;

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOldA = Visualizer_aVArrayList[ArrayId].aAttribute[iPosA];
	uint8_t AttrOldB = Visualizer_aVArrayList[ArrayId].aAttribute[iPosB];

	// Swap the values
	RendererWcc_DrawItem(ArrayId, iPosA, aArray[iPosA], AR_ATTR_WRITE);
	RendererWcc_DrawItem(ArrayId, iPosB, aArray[iPosB], AR_ATTR_WRITE);
	Visualizer_Sleep(fSleepMultiplier);
	RendererWcc_DrawItem(ArrayId, iPosA, aArray[iPosB], AttrOldA);
	RendererWcc_DrawItem(ArrayId, iPosB, aArray[iPosA], AttrOldB);

	return;

}

// Pointer:
// Highlight an item and keep it highlighted until the caller removes the highlight.


static uint8_t Visualizer_IsPointerOverlapped(intptr_t ArrayId, uint16_t PointerId) {

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

void Visualizer_UpdatePointer(intptr_t ArrayId, uint16_t PointerId, intptr_t iPos, double fSleepMultiplier) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (iPos >= Size) return;
	if (PointerId >= nPointer) return;

	if (
		(aPointer[PointerId] != (-1)) &&
		(!Visualizer_IsPointerOverlapped(ArrayId, PointerId))
		) {
		// Reset old pointer to normal.
		RendererWcc_DrawItem(ArrayId, aPointer[PointerId], aArray[aPointer[PointerId]], AR_ATTR_NORMAL);
	}

	RendererWcc_DrawItem(ArrayId, iPos, aArray[iPos], AR_ATTR_POINTER);
	Visualizer_Sleep(fSleepMultiplier);
	aPointer[PointerId] = iPos;

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, uint16_t PointerId) {

	if (!Visualizer_bInitialized) return;

	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (PointerId >= nPointer) return;

	if (!Visualizer_IsPointerOverlapped(ArrayId, PointerId)) {
		// Reset old pointer to normal.
		RendererWcc_DrawItem(ArrayId, aPointer[PointerId], aArray[aPointer[PointerId]], AR_ATTR_NORMAL);
	}
	aPointer[PointerId] = (intptr_t)(-1);

	return;

}

#else
#endif
