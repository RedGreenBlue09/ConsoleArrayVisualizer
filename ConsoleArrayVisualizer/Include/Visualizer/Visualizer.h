#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "Utils/MemoryPool.h"

// TODO: Hide internal stuff
// TODO: Make array resize work

typedef int32_t isort_t;
typedef uint32_t usort_t;

typedef void* Visualizer_Handle;

typedef uint8_t Visualizer_MarkerAttribute;
#define Visualizer_MarkerAttribute_Read 0
#define Visualizer_MarkerAttribute_Write 1
#define Visualizer_MarkerAttribute_Pointer 2
#define Visualizer_MarkerAttribute_Correct 3
#define Visualizer_MarkerAttribute_Incorrect 4
#define Visualizer_MarkerAttribute_EnumCount 5

#include "Visualizer/Renderer/ColumnWindowsConsole.h"

// Visualizer.c

#ifndef VISUALIZER_DISABLED

// Initialization

void Visualizer_Initialize();
void Visualizer_Uninitialize();

// Sleep
//#define VISUALIZER_DISABLE_SLEEP 1
#ifdef VISUALIZER_DISABLE_SLEEP
#define Visualizer_Sleep(X) 
#else
void Visualizer_Sleep(double fSleepMultiplier);
#endif

// Array

Visualizer_Handle Visualizer_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void Visualizer_RemoveArray(
	Visualizer_Handle hArray
);
void Visualizer_UpdateArrayState(
	Visualizer_Handle hArray,
	isort_t* aState
);
/*
void Visualizer_UpdateArray(
	Visualizer_Handle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);
*/
// Read

void Visualizer_UpdateRead(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	double fSleepMultiplier
);
void Visualizer_UpdateRead2(
	Visualizer_Handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
);
void Visualizer_UpdateReadMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
);

// Write

void Visualizer_UpdateWrite(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	isort_t NewValue,
	double fSleepMultiplier
);
void Visualizer_UpdateWrite2(
	Visualizer_Handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
);
void Visualizer_UpdateWriteMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
);

typedef Visualizer_Marker Visualizer_Pointer;

// Pointer
Visualizer_Pointer Visualizer_CreatePointer(
	Visualizer_Handle hArray,
	intptr_t iPosition
);
void Visualizer_RemovePointer(
	Visualizer_Pointer Pointer
);
void Visualizer_MovePointer(
	Visualizer_Pointer* pPointer,
	intptr_t iNewPosition
);

#else

// Define fake functions
#define Visualizer_Initialize()
#define Visualizer_Uninitialize()
#define Visualizer_Sleep(A)
#define Visualizer_AddArray(A)
#define Visualizer_RemoveArray(A)
#define Visualizer_UpdateArray(A, B, C, D, E, F)
#define Visualizer_UpdateRead(A, B, C)
#define Visualizer_UpdateRead2(A, B, C, D)
#define Visualizer_UpdateWrite(A, B, C, D)
#define Visualizer_UpdateWrite2(A, B, C, D, E, F)
#define Visualizer_UpdateSwap(A, B, C, D)
#define Visualizer_UpdatePointer(A, B, C)
#define Visualizer_RemovePointer(A, B)
#define Visualizer_UpdatePointerAuto(A, B, C)
#define Visualizer_RemovePointerAuto(A, B)

#endif

