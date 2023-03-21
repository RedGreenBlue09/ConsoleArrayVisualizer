#pragma once

#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>
#include "Tree234.h"
#include "DynamicArray.h"

typedef int32_t isort_t;
typedef uint32_t usort_t;

#define AV_MAX_ARRAY_COUNT   (16)
#define AV_MAX_POINTER_COUNT (256)

#define AV_RENDERER_UPDATEVALUE (0x01)
#define AV_RENDERER_UPDATEATTR  (0x02)

typedef enum {
	AvAttribute_Background,
	AvAttribute_Normal,
	AvAttribute_Read,
	AvAttribute_Write,
	AvAttribute_Pointer,
	AvAttribute_Correct,
	AvAttribute_Incorrect
} AvAttribute;

// Array properties struct

typedef struct {

	// Is the array ready for use
	int32_t         bActive;

	// Size of arrays in term of elements
	intptr_t        Size;

	// Tree
	// List of existing unique markers.
	tree234*        ptreeUniqueMarker;
	// Tree
	// Used to generate new id for unique markers.
	// Contains the starting positions of empty chunk of ids.
	tree234*        ptreeUniqueMarkerEmptyId;
	// Array of trees (used as max heaps)
	// Used to deal with overlapping unique markers.
	// The i'th position contains a list of markers
	// pointing to i at the same time.
	tree234**       aptreeUniqueMarkerMap;

	tree234*        ptreePointer;

} AV_ARRAYPROP;

typedef struct {

	intptr_t     Size;
	isort_t*     aArrayState;
	AvAttribute* aAttribute;

	int32_t      bVisible;
	isort_t      ValueMin;
	isort_t      ValueMax;

} AV_ARRAYPROP_RENDERER;

// Visualizer.c

// Low level renderer functions.
//#define VISUALIZER_DISABLED

#ifndef VISUALIZER_DISABLED

void Visualizer_Initialize();
void Visualizer_Uninitialize();

void Visualizer_Sleep(double fSleepMultiplier);

void Visualizer_AddArray(intptr_t ArrayId, intptr_t Size);
void Visualizer_RemoveArray(intptr_t ArrayId);
void Visualizer_UpdateArray(
	intptr_t ArrayId,
	isort_t NewSize,
	isort_t* aNewArrayState,
	int32_t bVisible,
	isort_t ValueMin,
	isort_t ValueMax
);

void Visualizer_UpdateRead(intptr_t ArrayId, intptr_t iPos, double fSleepMultiplier);
void Visualizer_UpdateRead2(intptr_t ArrayId, intptr_t iPosA, intptr_t iPosB, double fSleepMultiplier);
void Visualizer_UpdateWrite(intptr_t ArrayId, intptr_t iPos, isort_t NewValue, double fSleepMultiplier);
void Visualizer_UpdateWrite2(
	intptr_t ArrayId,
	intptr_t iPosA,
	intptr_t iPosB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
);

void Visualizer_UpdatePointer(intptr_t ArrayId, intptr_t PointerId, intptr_t iNewPos, double fSleepMultiplier);
void Visualizer_RemovePointer(intptr_t ArrayId, intptr_t PointerId);

#else

// Define fake functions
#define Visualizer_Initialize()
#define Visualizer_Uninitialize()
#define Visualizer_Sleep(A)
#define Visualizer_AddArray(A, B)
#define Visualizer_RemoveArray(A)
#define Visualizer_UpdateArray(A, B, C, D, E, F)
#define Visualizer_UpdateRead(A, B, C)
#define Visualizer_UpdateRead2(A, B, C, D)
#define Visualizer_UpdateWrite(A, B, C, D)
#define Visualizer_UpdateSwap(A, B, C, D)
#define Visualizer_UpdatePointer(A, B, C, D)
#define Visualizer_RemovePointer(A, B)

#endif

// Column_WindowsConsole.c

void RendererWcc_Initialize();
void RendererWcc_Uninitialize();

void RendererWcc_AddArray(intptr_t ArrayId, intptr_t Size);
void RendererWcc_RemoveArray(intptr_t ArrayId);
void RendererWcc_UpdateArray(
	intptr_t ArrayId,
	isort_t NewSize,
	isort_t* aNewArrayState,
	int32_t bVisible,
	isort_t ValueMin,
	isort_t ValueMax
);



/*
 * UpdateRequest:
 *   AV_RENDERER_UPDATEVALUE
 *   AV_RENDERER_UPDATEATTR
 *
 */

void RendererWcc_UpdateItem(
	intptr_t ArrayId,
	uintptr_t iPos,
	uint32_t UpdateRequest,
	isort_t NewValue,
	AvAttribute NewAttr
);

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
