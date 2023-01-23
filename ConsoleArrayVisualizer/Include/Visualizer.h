
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

// V_ARRAY
typedef struct {

	int32_t   bActive;

	intptr_t  Size;
	isort_t*  aArray;
	uint8_t*  aAttribute;

	int32_t   bVisible;
	isort_t   ValueMin;
	isort_t   ValueMax;

	intptr_t  nPointer;
	intptr_t* aPointer;
	// TODO: Tree stucture to store pointers

} V_ARRAY;

// Visualizer.c

// Low level renderer functions.
//#define VISUALIZER_DISABLED

#ifndef VISUALIZER_DISABLED

void Visualizer_Initialize();
void Visualizer_Uninitialize();

void Visualizer_UpdateItem(intptr_t ArrayId, intptr_t iPos, isort_t Value, uint8_t Attr);

void Visualizer_Sleep(double fSleepMultiplier);

void Visualizer_AddArray(intptr_t ArrayId, isort_t* aArray, intptr_t Size);
void Visualizer_RemoveArray(intptr_t ArrayId);
void Visualizer_UpdateArray(intptr_t ArrayId, int32_t bVisible, isort_t ValueMin, isort_t ValueMax);

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier);
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier);
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t Value, double fSleepMultiplier);
void Visualizer_UpdateSwap(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier);

void Visualizer_UpdatePointer(intptr_t ArrayId, uint16_t PointerId, intptr_t iPos, double fSleepMultiplier);
void Visualizer_RemovePointer(intptr_t ArrayId, uint16_t PointerId);

#else

#define arUpdateItem(A, B, C, D) 
#define arReadItemAttr(A, B, C) 

#define arInit() 
#define arUninit(A) 

#define arSleep(A, B, C, D) 

#define Visualizer_AddArray(A, B, C, D) 
#define Visualizer_RemoveArray(A) 
#define arSetRange(A, B) 
#define Visualizer_UpdateArray(A) 

#define Visualizer_UpdateRead(A, B, C) 
#define Visualizer_UpdateRead2(A, B, C, D) 
#define Visualizer_UpdateWrite(A, B, C, D) 
#define Visualizer_UpdateSwap(A, B, C, D) 

#define Visualizer_UpdatePointer(A, B, C, D) 
#define Visualizer_RemovePointer(A, B) 

#endif


// WindowsConsole.c

void WinConsole_FillStr(HANDLE hBuffer, CHAR* str, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillChar(HANDLE hBuffer, CHAR ch, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillAttr(HANDLE hBuffer, WORD Attr, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillAttrs(HANDLE hBuffer, WORD* attrs, SHORT wX, SHORT wY, COORD coordLocation);

void WinConsole_WriteStr(HANDLE hBuffer, CHAR* str, COORD coordLocation, ULONG ulLen);
void WinConsole_WriteChar(HANDLE hBuffer, CHAR ch, COORD coordLocation, ULONG ulLen);
void WinConsole_WriteAttr(HANDLE hBuffer, USHORT Attr, COORD coordLocation, ULONG ulLen);

void WinConsole_Clear(HANDLE hBuffer);
void WinConsole_Pause();

HANDLE* WinConsole_CreateBuffer();
void WinConsole_FreeBuffer(HANDLE hBuffer);


// Column_WindowsConsole.c

void RendererWcc_Initialize();
void RendererWcc_Uninitialize();

void RendererWcc_AddArray(V_ARRAY* parArray, intptr_t id);
void RendererWcc_RemoveArray(intptr_t id);

void RendererWcc_DrawItem(intptr_t ArrayId, uintptr_t iPos, isort_t Value, uint8_t Attr);
