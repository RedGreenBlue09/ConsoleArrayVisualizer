
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


void arInit();
void arUninit();

void arUpdateItem(isort_t value, uintptr_t n, uintptr_t pos, uint8_t attr);
void arReadItemAttr(isort_t value, uintptr_t n, uintptr_t pos, uint8_t* pAttr);
// Useful operations

void arSleep(double multiplier);
void arUpdateArray(isort_t* array, uintptr_t n);

void arUpdateRead(isort_t* array, uintptr_t n, uintptr_t pos, double sleepMultiplier);
void arUpdateRead2(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier);
void arUpdateWrite(isort_t* array, uintptr_t n, uintptr_t pos, isort_t value, double sleepMultiplier);
void arUpdateSwap(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier);

uintptr_t prevPointers[256];

void arUpdatePointer(isort_t* array, uintptr_t n, uintptr_t pos, uint16_t pointerId, double sleepMultiplier);
void arRemovePointer(isort_t* array, uintptr_t n, uint16_t pointerId);

// ColumnRenderer.c

void arcnclInit();
void arcnclUninit();
void arcnclDrawItem(isort_t value, uintptr_t n, uintptr_t pos, uint8_t attr);
void arcnclReadItemAttr(isort_t value, uintptr_t n, uintptr_t pos, uint8_t* pAttr);


