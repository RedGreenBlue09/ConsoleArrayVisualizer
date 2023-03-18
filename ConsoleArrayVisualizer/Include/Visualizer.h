
#include <Windows.h>

#include "Utils.h"
#include "Sorts.h"

typedef enum VisualizerAttribute_tag VisualizerAttribute;
typedef struct V_ARRAY_tag V_ARRAY;

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
void Visualizer_UpdateSwap(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, isort_t ValueA, isort_t ValueB, double fSleepMultiplier);

void Visualizer_UpdatePointer(intptr_t ArrayId, intptr_t PointerId, intptr_t iNewPos, double fSleepMultiplier);
void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId);

#else

// Define fake functions

#endif

// Column_WindowsConsole.c

void RendererWcc_Initialize();
void RendererWcc_Uninitialize();

void RendererWcc_AddArray(intptr_t ArrayId, V_ARRAY* pVArray);
void RendererWcc_RemoveArray(intptr_t ArrayId);
void RendererWcc_UpdateArray(intptr_t ArrayId, int32_t bVisible, isort_t ValueMin, isort_t ValueMax);

void RendererWcc_DrawItem(intptr_t ArrayId, uintptr_t iPos, isort_t Value, uint8_t Attr);

// WindowsConsole.c

void WinConsole_FillStr(HANDLE hBuffer, CHAR* str, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillChar(HANDLE hBuffer, CHAR ch, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillAttr(HANDLE hBuffer, CONSOLE_SCREEN_BUFFER_INFOEX* pCSBI, WORD Attr, SHORT wX, SHORT wY, COORD coordLocation);
void WinConsole_FillAttrs(HANDLE hBuffer, WORD* attrs, SHORT wX, SHORT wY, COORD coordLocation);

void WinConsole_WriteStr(HANDLE hBuffer, CHAR* str, COORD coordLocation, ULONG ulLen);
void WinConsole_WriteChar(HANDLE hBuffer, CHAR ch, COORD coordLocation, ULONG ulLen);
void WinConsole_WriteAttr(HANDLE hBuffer, USHORT Attr, COORD coordLocation, ULONG ulLen);

void WinConsole_Clear(HANDLE hBuffer);
void WinConsole_Pause();

HANDLE* WinConsole_CreateBuffer();
void WinConsole_FreeBuffer(HANDLE hBuffer);
