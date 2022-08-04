
#include "ArrayRenderer.h"

const uint64_t defaultSleepTime = 1000;
isort_t valueMax = 64;

// Low level renderer functions

uintptr_t myPointersN = 256;
uintptr_t myPointers[256];

void arInit() {
	arcnclInit();
	for (uintptr_t i = 0; i < myPointersN; ++i)
		myPointers[i] = (uintptr_t)(-1);
	return;
}

void arUninit() {
	arcnclUninit();
	return;
}

void arUpdateItem(isort_t value, uintptr_t n, uintptr_t pos, uint8_t attr) {
	arcnclDrawItem(value, n, pos, attr);
}

void arReadItemAttr(isort_t value, uintptr_t n, uintptr_t pos, uint8_t* pAttr) {
	arcnclReadItemAttr(value, n, pos, pAttr);
}

// Useful operations

void arSleep(double multiplier) {
	sleep64((uint64_t)((double)defaultSleepTime * multiplier));
	return;
}

void arUpdateArray(isort_t* array, uintptr_t n) {
	for (uintptr_t i = 0; i < n; ++i)
		arUpdateItem(array[i], n, i, AR_ATTR_NORMAL);
}

// These functions restore original attributes before they return.
void arUpdateRead(isort_t* array, uintptr_t n, uintptr_t pos, double sleepMultiplier) {

	uint8_t oldAttr;
	arReadItemAttr(array[pos], n, pos, &oldAttr);

	arUpdateItem(array[pos], n, pos, AR_ATTR_READ);
	arSleep(sleepMultiplier);
	arUpdateItem(array[pos], n, pos, oldAttr);
}

// Update 2 items with a single sleep (used for comparisions)
void arUpdateRead2(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier) {

	uint8_t oldAttrA;
	uint8_t oldAttrB;
	arReadItemAttr(array[posA], n, posA, &oldAttrA);
	arReadItemAttr(array[posB], n, posB, &oldAttrB);

	arUpdateItem(array[posA], n, posA, AR_ATTR_READ);
	arUpdateItem(array[posB], n, posB, AR_ATTR_READ);
	arSleep(sleepMultiplier);
	arUpdateItem(array[posA], n, posA, oldAttrA);
	arUpdateItem(array[posB], n, posB, oldAttrB);
}

// For time precision, the sort will need to do the write(s) by itself.
void arUpdateWrite(isort_t* array, uintptr_t n, uintptr_t pos, isort_t value, double sleepMultiplier) {

	uint8_t oldAttr;
	arReadItemAttr(array[pos], n, pos, &oldAttr);

	arUpdateItem(array[pos], n, pos, AR_ATTR_WRITE);
	arSleep(sleepMultiplier);
	arUpdateItem(value, n, pos, oldAttr);
}

void arUpdateSwap(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier) {

	uint8_t oldAttrA;
	uint8_t oldAttrB;
	arReadItemAttr(array[posA], n, posA, &oldAttrA);
	arReadItemAttr(array[posB], n, posB, &oldAttrB);

	arUpdateItem(array[posA], n, posA, AR_ATTR_WRITE);
	arUpdateItem(array[posB], n, posB, AR_ATTR_WRITE);
	arSleep(sleepMultiplier);
	// Swap the values
	arUpdateItem(array[posB], n, posA, oldAttrA);
	arUpdateItem(array[posA], n, posB, oldAttrB);

}

// These functions remember position,
// and only them can leave highlights after they return.
// Useful for pointer visualization.

// 256 pointers at max. TODO: Dynamic array

static uint8_t arIsPointerOverlapped(uint16_t pointerId) {
	uint8_t isOverlapping = FALSE;
	for (uintptr_t i = 0; i < myPointersN; ++i) {
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

void arUpdatePointer(isort_t* array, uintptr_t n, uintptr_t pos, uint16_t pointerId, double sleepMultiplier) {

	if (
		(myPointers[pointerId] != (uintptr_t)(-1)) &&
		(!arIsPointerOverlapped(pointerId))
		) {
		// Reset old pointer to normal.
		arUpdateItem(array[myPointers[pointerId]], n, myPointers[pointerId], AR_ATTR_NORMAL);
	}

	myPointers[pointerId] = pos;
	arUpdateItem(array[pos], n, pos, AR_ATTR_POINTER);
	arSleep(sleepMultiplier);
}

void arRemovePointer(isort_t* array, uintptr_t n, uint16_t pointerId) {
	if (!arIsPointerOverlapped(pointerId)) {
		// Reset old pointer to normal.
		arUpdateItem(array[myPointers[pointerId]], n, myPointers[pointerId], AR_ATTR_NORMAL);
	}
	myPointers[pointerId] = (uintptr_t)(-1);
}
