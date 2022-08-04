
#include "ArrayRenderer.h"

const uint64_t defaultSleepTime = 1000;
isort_t valueMax = 64;

// Low level renderer functions

void arInit() {
	arcnclInit();
	return;
}

void arUninit() {
	arcnclUninit();
	return;
}

void arUpdateItem(isort_t* array, uintptr_t n, uintptr_t pos, uint8_t attr) {
	arcnclDrawItem(array, n, pos, attr);
}

void arReadItemAttr(isort_t* array, uintptr_t n, uintptr_t pos, uint8_t* pAttr) {
	arcnclReadItemAttr(array, n, pos, pAttr);
}

// Useful operations

void arSleep(double multiplier) {
	sleep64((uint64_t)((double)defaultSleepTime * multiplier));
	return;
}

void arUpdateArray(isort_t* array, uintptr_t n) {
	for (uintptr_t i = 0; i < n; ++i)
		arUpdateItem(array, n, i, AR_ATTR_NORMAL);
}

void arUpdateRead(isort_t* array, uintptr_t n, uintptr_t pos, double sleepMultiplier) {

	uint8_t oldAttr;
	arReadItemAttr(array, n, pos, &oldAttr);

	arUpdateItem(array, n, pos, AR_ATTR_READ);
	arSleep(sleepMultiplier);
	arUpdateItem(array, n, pos, oldAttr);
}

// Update 2 items with a single sleep (used for comparisions)
void arUpdateRead2(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier) {

	uint8_t oldAttrA;
	arReadItemAttr(array, n, posA, &oldAttrA);
	uint8_t oldAttrB;
	arReadItemAttr(array, n, posB, &oldAttrB);

	arUpdateItem(array, n, posA, AR_ATTR_READ);
	arUpdateItem(array, n, posB, AR_ATTR_READ);
	arSleep(sleepMultiplier);
	arUpdateItem(array, n, posA, oldAttrA);
	arUpdateItem(array, n, posB, oldAttrB);
}

void arUpdateWrite(isort_t* array, uintptr_t n, uintptr_t pos, isort_t value, double sleepMultiplier) {

	uint8_t oldAttr;
	arReadItemAttr(array, n, pos, &oldAttr);

	arUpdateItem(array, n, pos, AR_ATTR_WRITE);
	arSleep(sleepMultiplier);
	arUpdateItem(array, n, pos, oldAttr);
}

void arUpdateSwap(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier) {

	uint8_t oldAttrA;
	arReadItemAttr(array, n, posA, &oldAttrA);
	uint8_t oldAttrB;
	arReadItemAttr(array, n, posB, &oldAttrB);

	arUpdateItem(array, n, posA, AR_ATTR_WRITE);
	arUpdateItem(array, n, posB, AR_ATTR_WRITE);
	ISORT_SWAP(array[posA], array[posB]);
	arSleep(sleepMultiplier);
	arUpdateItem(array, n, posA, oldAttrA);
	arUpdateItem(array, n, posB, oldAttrB);

}

// These functions remember position. Useful for pointer visualization.

// 256 pointers at max. TODO: Dynamic array
uintptr_t prevPointers[256];

void arAddPointer(isort_t* array, uintptr_t n, uintptr_t pos, uint16_t pointerId, double sleepMultiplier) {

	prevPointers[pointerId] = pos;
	arUpdateItem(array, n, pos, AR_ATTR_POINTER);
	arSleep(sleepMultiplier);
}

void arUpdatePointer(isort_t* array, uintptr_t n, uintptr_t pos, uint16_t pointerId, double sleepMultiplier) {

	// Reset to normal. TODO: handle pointer overlapping
	arUpdateItem(array, n, prevPointers[pointerId], AR_ATTR_NORMAL);

	prevPointers[pointerId] = pos;
	arUpdateItem(array, n, pos, AR_ATTR_POINTER);
	arSleep(sleepMultiplier);
}
