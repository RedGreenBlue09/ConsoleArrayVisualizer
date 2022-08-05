
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
void arcnclDrawItem(isort_t value, uintptr_t n, uintptr_t pos, uint8_t attr);
void arcnclReadItemAttr(isort_t value, uintptr_t n, uintptr_t pos, uint8_t* pAttr);

// Visualizer.c

const uint64_t defaultSleepTime;
isort_t valueMax;

void arInit();
void arUninit();

void arUpdateItem(isort_t value, uintptr_t n, uintptr_t pos, uint8_t attr);
void arReadItemAttr(isort_t value, uintptr_t n, uintptr_t pos, uint8_t* pAttr);
void arSetRange(isort_t newRange);

void arSleep(double multiplier);
void arUpdateArray(isort_t* array, uintptr_t n);

void arUpdateRead(isort_t* array, uintptr_t n, uintptr_t pos, double sleepMultiplier);
void arUpdateRead2(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier);
void arUpdateWrite(isort_t* array, uintptr_t n, uintptr_t pos, isort_t value, double sleepMultiplier);
void arUpdateSwap(isort_t* array, uintptr_t n, uintptr_t posA, uintptr_t posB, double sleepMultiplier);

uintptr_t prevPointers[256];

void arUpdatePointer(isort_t* array, uintptr_t n, uintptr_t pos, uint16_t pointerId, double sleepMultiplier);
void arRemovePointer(isort_t* array, uintptr_t n, uint16_t pointerId);



