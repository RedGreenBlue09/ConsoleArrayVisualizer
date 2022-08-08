
#include "Visualizer.h"

const uint64_t arDefaultSleepTime = 250;
uint8_t arInitialized = FALSE;

//
AR_ARRAY arArrayList[AR_MAX_ARRAY_COUNT];

// Low level renderer functions.

void arUpdateItem(intptr_t arrayId, intptr_t pos, isort_t value, uint8_t attr) {
	if (!arInitialized) return;
	arcnclDrawItem(arrayId, pos, value, attr);
}

void arReadItemAttr(intptr_t arrayId, intptr_t pos, uint8_t* pAttr) {
	if (!arInitialized) return;
	arcnclReadItemAttr(arrayId, pos, pAttr);
}

// Init.

void arInit() {

	arcnclInit();

	for (intptr_t i = 0; i < myPointersN; ++i)
		myPointers[i] = (intptr_t)(-1);

	memset(arArrayList, 0, sizeof(arArrayList));
	// This also set all .active to FALSE

	arInitialized = TRUE;
	return;
}

void arUninit() {
	arcnclUninit();
	arInitialized = FALSE;
	return;
}

// Sleep.

void arSleep(double multiplier) {
	if (!arInitialized) return;
	sleep64((uint64_t)((double)arDefaultSleepTime * multiplier));
	return;
}

// Array.

void arAddArray(intptr_t arrayId, isort_t* array, intptr_t n, isort_t valueMax) {
	arArrayList[arrayId].array = array;
	arArrayList[arrayId].n = n;
	arArrayList[arrayId].valueMax = valueMax;
	arArrayList[arrayId].active = TRUE;

	arcnclAddArray(arrayId);

	return;
}

void arRemoveArray(intptr_t arrayId) {

	arcnclRemoveArray(arrayId);

	memset(&arArrayList[arrayId], 0, sizeof(arArrayList[arrayId]));
	// This also set all .active to FALSE
	return;
}

void arSetRange(intptr_t arrayId, isort_t newRange) {
	arArrayList[arrayId].valueMax = newRange;
}

void arUpdateArray(intptr_t arrayId) {
	isort_t* array = arArrayList[arrayId].array;
	intptr_t n = arArrayList[arrayId].n;

	if (!arInitialized) return;
	for (intptr_t i = 0; i < n; ++i) {
		arUpdateItem(arrayId, i, array[i], AR_ATTR_NORMAL);
	}
}

// Read & Write.
// These functions restore original attributes before they return.

void arUpdateRead(intptr_t arrayId, intptr_t pos, double sleepMultiplier) {
	isort_t* array = arArrayList[arrayId].array;
	intptr_t n = arArrayList[arrayId].n;

	uint8_t oldAttr;
	arReadItemAttr(arrayId, pos, &oldAttr);

	arUpdateItem(arrayId, pos, array[pos], AR_ATTR_READ);
	arSleep(sleepMultiplier);
	arUpdateItem(arrayId, pos, array[pos], oldAttr);
}

// Update 2 items with a single sleep (used for comparisions)
void arUpdateRead2(intptr_t arrayId, intptr_t posA, intptr_t posB, double sleepMultiplier) {
	isort_t* array = arArrayList[arrayId].array;
	intptr_t n = arArrayList[arrayId].n;

	uint8_t oldAttrA;
	uint8_t oldAttrB;
	arReadItemAttr(arrayId, posA, &oldAttrA);
	arReadItemAttr(arrayId, posB, &oldAttrB);

	arUpdateItem(arrayId, posA, array[posA], AR_ATTR_READ);
	arUpdateItem(arrayId, posB, array[posB], AR_ATTR_READ);
	arSleep(sleepMultiplier);
	arUpdateItem(arrayId, posA, array[posA], oldAttrA);
	arUpdateItem(arrayId, posB, array[posB], oldAttrB);
}

// For time precision, the sort will need to do the write(s) by itself.
void arUpdateWrite(intptr_t arrayId, intptr_t pos, isort_t value, double sleepMultiplier) {
	isort_t* array = arArrayList[arrayId].array;
	intptr_t n = arArrayList[arrayId].n;

	uint8_t oldAttr;
	arReadItemAttr(arrayId, pos, &oldAttr);

	arUpdateItem(arrayId, pos, value, AR_ATTR_WRITE);
	arSleep(sleepMultiplier);
	arUpdateItem(arrayId, pos, value, oldAttr);
}

void arUpdateSwap(intptr_t arrayId, intptr_t posA, intptr_t posB, double sleepMultiplier) {
	isort_t* array = arArrayList[arrayId].array;
	intptr_t n = arArrayList[arrayId].n;

	uint8_t oldAttrA;
	uint8_t oldAttrB;
	arReadItemAttr(arrayId, posA, &oldAttrA);
	arReadItemAttr(arrayId, posB, &oldAttrB);

	// Swap the values
	arUpdateItem(arrayId, posA, array[posB], AR_ATTR_WRITE);
	arUpdateItem(arrayId, posB, array[posA], AR_ATTR_WRITE);
	arSleep(sleepMultiplier);

	arUpdateItem(arrayId, posA, array[posB], oldAttrA);
	arUpdateItem(arrayId, posB, array[posA], oldAttrB);

}

// Pointer (it means a variable that holds index).
// These functions remember position,
// and only them can leave highlights after they return.
// Useful for pointer visualization.

intptr_t myPointersN = AR_MAX_POINTER_COUNT;
intptr_t myPointers[AR_MAX_POINTER_COUNT];
// TODO: Tree stucture to store pointers

static uint8_t arIsPointerOverlapped(uint16_t pointerId) {
	uint8_t isOverlapping = FALSE;
	for (intptr_t i = 0; i < myPointersN; ++i) {
		if (
			(i != pointerId) &&
			(myPointers[i] == myPointers[pointerId])
			) {
			isOverlapping = TRUE;
			break;
		}
	}
	return isOverlapping;
}

void arUpdatePointer(intptr_t arrayId, uint16_t pointerId, intptr_t pos, double sleepMultiplier) {
	if (!arInitialized) return;

	isort_t* array = arArrayList[arrayId].array;
	intptr_t n = arArrayList[arrayId].n;

	if (pos >= n) return;
	if (pointerId >= myPointersN) return;

	if (
		(myPointers[pointerId] != (intptr_t)(-1)) &&
		(!arIsPointerOverlapped(pointerId))
		) {
		// Reset old pointer to normal.
		arUpdateItem(arrayId, myPointers[pointerId], array[myPointers[pointerId]], AR_ATTR_NORMAL);
	}

	arUpdateItem(arrayId, pos, array[pos], AR_ATTR_POINTER);
	arSleep(sleepMultiplier);
	myPointers[pointerId] = pos;
}

void arRemovePointer(intptr_t arrayId, uint16_t pointerId) {
	if (!arInitialized) return;

	isort_t* array = arArrayList[arrayId].array;
	intptr_t n = arArrayList[arrayId].n;

	if (!arIsPointerOverlapped(pointerId)) {
		// Reset old pointer to normal.
		arUpdateItem(arrayId, myPointers[pointerId], array[myPointers[pointerId]], AR_ATTR_NORMAL);
	}
	myPointers[pointerId] = (intptr_t)(-1);
}
