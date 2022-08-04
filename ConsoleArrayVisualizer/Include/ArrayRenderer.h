
#include <Windows.h>

#include "Utils.h"
#include "Sorts.h"

// TODO: enum
#define AR_ATTR_BACKGROUND (0)
#define AR_ATTR_NORMAL (1)
#define AR_ATTR_READ (2)
#define AR_ATTR_WRITE (3)
#define AR_ATTR_POINTER (4)

// Source.c

const uint64_t defaultSleepTime;
isort_t valueMax;


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

void arSleep(double multiplier);

void arUpdateArray(isort_t* array, uintptr_t n);
void arUpdateRead(isort_t* array, uintptr_t n, uintptr_t pos, double sleepMultiplier);

// Update 2 items with a single sleep (used for comparisions)
void arUpdateRead2(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier);
void arUpdateWrite(isort_t* array, uintptr_t n, uintptr_t pos, isort_t value, double sleepMultiplier);
void arUpdateSwap(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier);

uintptr_t prevPointers[256];

void arAddPointer(isort_t* array, uintptr_t n, uintptr_t pos, uint16_t pointerId, double sleepMultiplier);
void arUpdatePointer(isort_t* array, uintptr_t n, uintptr_t pos, uint16_t pointerId, double sleepMultiplier);

// ColumnRenderer.c

void arcnclInit();
void arcnclUninit();
void arcnclDrawItem(isort_t* array, uintptr_t n, uintptr_t pos, uint8_t attr);
void arcnclReadItemAttr(isort_t* array, uintptr_t n, uintptr_t pos, uint8_t* pAttr);


