
#include <Windows.h>

#include "Utils.h"
#include "Sorts.h"

// TODO: enum
#define AR_ATTR_BACKGROUND (0)
#define AR_ATTR_NORMAL (1)
#define AR_ATTR_READ (2)
#define AR_ATTR_WRITE (3)
#define AR_ATTR_POINTER (4)
#define AR_ATTR_CORRECT (5)
#define AR_ATTR_INCORRECT (6)

#define AR_MAX_ARRAY_COUNT (16)
#define AR_MAX_POINTER_COUNT (256)

// ARRAY_ITEM
typedef struct {
	uint8_t active; // boolean
	isort_t* array;
	intptr_t n;
	isort_t valueMax; // aka. range
} AR_ARRAY;

// WindowsConsole.c

void cnFillStr(HANDLE hBuffer, CHAR* str, SHORT wX, SHORT wY, COORD coordLocation);
void cnFillChar(HANDLE hBuffer, CHAR ch, SHORT wX, SHORT wY, COORD coordLocation);
void cnFillAttr(HANDLE hBuffer, WORD attr, SHORT wX, SHORT wY, COORD coordLocation);
void cnFillAttrs(HANDLE hBuffer, WORD* attrs, SHORT wX, SHORT wY, COORD coordLocation);

void cnWriteStr(HANDLE hBuffer, CHAR* str, COORD coordLocation, ULONG ulLen);
void cnWriteChar(HANDLE hBuffer, CHAR ch, COORD coordLocation, ULONG ulLen);
void cnWriteAttr(HANDLE hBuffer, USHORT attr, COORD coordLocation, ULONG ulLen);

void cnClear(HANDLE hBuffer);
void cnPause();

HANDLE* cnCreateBuffer();
void cnDeleteBuffer(HANDLE hBuffer);


// Column_WindowsConsole.c

void arcnclInit();
void arcnclUninit();

void arcnclAddArray(intptr_t id, isort_t* array, intptr_t n);
void arcnclRemoveArray(intptr_t id);

void arcnclDrawItem(intptr_t arrayId, isort_t value, uintptr_t pos, uint8_t attr);
void arcnclReadItemAttr(intptr_t arrayId, uintptr_t pos, uint8_t* pAttr);

// Visualizer.c

extern const uint64_t arDefaultSleepTime;
extern uint8_t arInitialized;

//
AR_ARRAY arArrayList[AR_MAX_ARRAY_COUNT];

// Low level renderer functions.

void arUpdateItem(intptr_t arrayId, intptr_t pos, isort_t value, uint8_t attr);
void arReadItemAttr(intptr_t arrayId, intptr_t pos, uint8_t* pAttr);

void arInit();
void arUninit();

void arSleep(double multiplier);

void arAddArray(intptr_t arrayId, isort_t* array, intptr_t n, isort_t valueMax);
void arRemoveArray(intptr_t arrayId);
void arSetRange(intptr_t arrayId, isort_t newRange);
void arUpdateArray(intptr_t arrayId);

void arUpdateRead(intptr_t arrayId, intptr_t pos, double sleepMultiplier);
void arUpdateRead2(intptr_t arrayId, intptr_t posA, intptr_t posB, double sleepMultiplier);
void arUpdateWrite(intptr_t arrayId, intptr_t pos, isort_t value, double sleepMultiplier);
void arUpdateSwap(intptr_t arrayId, intptr_t posA, intptr_t posB, double sleepMultiplier);

static uint8_t arIsPointerOverlapped(uint16_t pointerId);

extern intptr_t myPointersN;
extern intptr_t myPointers[];

void arUpdatePointer(intptr_t arrayId, uint16_t pointerId, intptr_t pos, double sleepMultiplier);
void arRemovePointer(intptr_t arrayId, uint16_t pointerId);
