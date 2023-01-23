
#include "Visualizer.h"

#ifndef VISUALIZER_DISABLED

static const uint64_t Visualizer_TimeDefaultDelay = 250;
static uint8_t Visualizer_bInitialized = FALSE;

//
static V_ARRAY Visualizer_aVArrayList[AR_MAX_ARRAY_COUNT];

// Low level renderer functions

void Visualizer_UpdateItem(intptr_t ArrayId, intptr_t iPos, isort_t Value, uint8_t Attr) {
	if (!Visualizer_bInitialized) return;
	arcnclDrawItem(ArrayId, iPos, Value, Attr);
	return;
}

void Visualizer_ReadItemAttribute(intptr_t ArrayId, intptr_t iPos, uint8_t* pAttr) {
	if (!Visualizer_bInitialized) return;
	arcnclReadItemAttr(ArrayId, iPos, pAttr);
	return;
}

// Init

void Visualizer_Initialize() {

	// Initialize Renderer
	arcnclInit();

	for (intptr_t i = 0; i < AR_MAX_ARRAY_COUNT; ++i) {

		Visualizer_aVArrayList[i].bActive = FALSE;

		Visualizer_aVArrayList[i].aArray = NULL;
		Visualizer_aVArrayList[i].Size = 0;

		Visualizer_aVArrayList[i].aAttribute = NULL;
		Visualizer_aVArrayList[i].nAttribute = 0;

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
	arcnclUninit();
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

void Visualizer_AddArray(intptr_t ArrayId, isort_t* aArray, intptr_t Size, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {

	Visualizer_aVArrayList[ArrayId].bActive = TRUE;

	Visualizer_aVArrayList[ArrayId].aArray = aArray;
	Visualizer_aVArrayList[ArrayId].Size = Size;

	Visualizer_aVArrayList[ArrayId].aAttribute = NULL;
	Visualizer_aVArrayList[ArrayId].nAttribute = Size;

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = ValueMin;
	Visualizer_aVArrayList[ArrayId].ValueMax = ValueMax;

	Visualizer_aVArrayList[ArrayId].nPointer = AR_MAX_POINTER_COUNT;
	Visualizer_aVArrayList[ArrayId].aPointer = malloc(AR_MAX_POINTER_COUNT * sizeof(intptr_t));

	arcnclAddArray(&Visualizer_aVArrayList[ArrayId], ArrayId);

	return;

}

void Visualizer_RemoveArray(intptr_t ArrayId) {

	arcnclRemoveArray(ArrayId);

	//
	Visualizer_aVArrayList[ArrayId].bActive = FALSE;

	Visualizer_aVArrayList[ArrayId].aArray = NULL;
	Visualizer_aVArrayList[ArrayId].Size = 0;

	Visualizer_aVArrayList[ArrayId].aAttribute = NULL;
	Visualizer_aVArrayList[ArrayId].nAttribute = 0;

	Visualizer_aVArrayList[ArrayId].bVisible = FALSE;
	Visualizer_aVArrayList[ArrayId].ValueMin = 0;
	Visualizer_aVArrayList[ArrayId].ValueMax = 0;

	Visualizer_aVArrayList[ArrayId].nPointer = AR_MAX_POINTER_COUNT;
	Visualizer_aVArrayList[ArrayId].aPointer = malloc(AR_MAX_POINTER_COUNT * sizeof(intptr_t));


	// This also set all .active to FALSE
	return;

}

void Visualizer_SetArrayVisibility(intptr_t ArrayId, int32_t bVisible, isort_t ValueMin, isort_t ValueMax) {
	Visualizer_aVArrayList[ArrayId].bVisible = bVisible;
	Visualizer_aVArrayList[ArrayId].ValueMin = ValueMin;
	Visualizer_aVArrayList[ArrayId].ValueMax = ValueMax;
	// TODO: In real time
	return;
}

void Visualizer_UpdateArray(intptr_t ArrayId) {

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	if (!Visualizer_bInitialized) return;

	for (intptr_t i = 0; i < Size; ++i) {
		Visualizer_UpdateItem(ArrayId, i, aArray[i], AR_ATTR_NORMAL);
	}

	return;

}

// Read & Write
// These functions restore original attributes before they return.

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier) {

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOld;
	Visualizer_ReadItemAttribute(ArrayId, iPos, &AttrOld);

	Visualizer_UpdateItem(ArrayId, iPos, aArray[iPos], AR_ATTR_READ);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_UpdateItem(ArrayId, iPos, aArray[iPos], AttrOld);

	return;

}

// Update 2 items with a single sleep (used for comparisions).
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOldA;
	uint8_t AttrOldB;
	Visualizer_ReadItemAttribute(ArrayId, iPosA, &AttrOldA);
	Visualizer_ReadItemAttribute(ArrayId, iPosB, &AttrOldB);

	Visualizer_UpdateItem(ArrayId, iPosA, aArray[iPosA], AR_ATTR_READ);
	Visualizer_UpdateItem(ArrayId, iPosB, aArray[iPosB], AR_ATTR_READ);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_UpdateItem(ArrayId, iPosA, aArray[iPosA], AttrOldA);
	Visualizer_UpdateItem(ArrayId, iPosB, aArray[iPosB], AttrOldB);

	return;

}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t Value, double fSleepMultiplier) {

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOld;
	Visualizer_ReadItemAttribute(ArrayId, iPos, &AttrOld);

	Visualizer_UpdateItem(ArrayId, iPos, Value, AR_ATTR_WRITE);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_UpdateItem(ArrayId, iPos, Value, AttrOld);

	return;

}

void Visualizer_UpdateSwap(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier) {

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;

	uint8_t AttrOldA;
	uint8_t AttrOldB;
	Visualizer_ReadItemAttribute(ArrayId, iPosA, &AttrOldA);
	Visualizer_ReadItemAttribute(ArrayId, iPosB, &AttrOldB);

	// Swap the values
	Visualizer_UpdateItem(ArrayId, iPosA, aArray[iPosB], AR_ATTR_WRITE);
	Visualizer_UpdateItem(ArrayId, iPosB, aArray[iPosA], AR_ATTR_WRITE);
	Visualizer_Sleep(fSleepMultiplier);

	Visualizer_UpdateItem(ArrayId, iPosA, aArray[iPosB], AttrOldA);
	Visualizer_UpdateItem(ArrayId, iPosB, aArray[iPosA], AttrOldB);

	return;

}

// Pointer:
// Highlight an item and keep it highlighted until the caller removes.


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

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (iPos >= Size) return;
	if (PointerId >= nPointer) return;

	if (
		(aPointer[PointerId] != (intptr_t)(-1)) &&
		(!Visualizer_IsPointerOverlapped(ArrayId, PointerId))
		) {
		// Reset old pointer to normal.
		Visualizer_UpdateItem(ArrayId, aPointer[PointerId], aArray[aPointer[PointerId]], AR_ATTR_NORMAL);
	}

	Visualizer_UpdateItem(ArrayId, iPos, aArray[iPos], AR_ATTR_POINTER);
	Visualizer_Sleep(fSleepMultiplier);
	aPointer[PointerId] = iPos;

	return;

}

void Visualizer_RemovePointer(intptr_t ArrayId, uint16_t PointerId) {

	if (!Visualizer_bInitialized) return;

	isort_t* aArray = Visualizer_aVArrayList[ArrayId].aArray;
	intptr_t Size = Visualizer_aVArrayList[ArrayId].Size;
	intptr_t nPointer = Visualizer_aVArrayList[ArrayId].nPointer;
	intptr_t* aPointer = Visualizer_aVArrayList[ArrayId].aPointer;

	if (PointerId >= nPointer) return;

	if (!Visualizer_IsPointerOverlapped(ArrayId, PointerId)) {
		// Reset old pointer to normal.
		Visualizer_UpdateItem(ArrayId, aPointer[PointerId], aArray[aPointer[PointerId]], AR_ATTR_NORMAL);
	}
	aPointer[PointerId] = (intptr_t)(-1);

	return;

}

#else
#endif
