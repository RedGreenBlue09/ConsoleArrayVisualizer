#pragma once

#include "Visualizer/Common.h"

// ColumnWindowsConsole.c

void RendererCwc_Initialize();
void RendererCwc_Uninitialize();

Visualizer_Handle RendererCwc_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void RendererCwc_RemoveArray(Visualizer_Handle hArray);
void RendererCwc_UpdateArrayState(Visualizer_Handle hArray, isort_t* aState);

typedef struct {
	Visualizer_Handle hArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_Pointer;

// Read

void RendererCwc_UpdateRead(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	double fSleepMultiplier
);
void RendererCwc_UpdateRead2(
	Visualizer_Handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
);
void RendererCwc_UpdateReadMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
);

// Write

void RendererCwc_UpdateWrite(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	isort_t NewValue,
	double fSleepMultiplier
);
void RendererCwc_UpdateWrite2(
	Visualizer_Handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
);
void RendererCwc_UpdateWriteMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
);

// Pointer

Visualizer_Pointer RendererCwc_CreatePointer(
	Visualizer_Handle hArray,
	intptr_t iPosition
);
void RendererCwc_RemovePointer(
	Visualizer_Pointer Pointer
);
void RendererCwc_MovePointer(
	Visualizer_Pointer* pPointer,
	intptr_t iNewPosition
);

void RendererCwc_SetAlgorithmName(char* sAlgorithmNameArg);
void RendererCwc_ClearReadWriteCounter(Visualizer_Handle hArray);

#define Visualizer_Initialize() RendererCwc_Initialize()
#define Visualizer_Uninitialize() RendererCwc_Uninitialize()

#define Visualizer_AddArray(Size, aArrayState, ValueMin, ValueMax) \
	RendererCwc_AddArray(Size, aArrayState, ValueMin, ValueMax)
#define Visualizer_RemoveArray(hArray) RendererCwc_RemoveArray(hArray)
#define Visualizer_UpdateArrayState(hArray, aState) RendererCwc_UpdateArrayState(hArray, aState)

#define Visualizer_UpdateRead(hArray, iPosition, fSleepMultiplier) \
	RendererCwc_UpdateRead(hArray, iPosition, fSleepMultiplier)
#define Visualizer_UpdateRead2(hArray, iPositionA, iPositionB, fSleepMultiplier) \
	RendererCwc_UpdateRead2(hArray, iPositionA, iPositionB, fSleepMultiplier)
#define Visualizer_UpdateReadMulti(hArray, iStartPosition, Length, fSleepMultiplier) \
	RendererCwc_UpdateReadMulti(hArray, iStartPosition, Length, fSleepMultiplier)

#define Visualizer_UpdateWrite(hArray, iPosition, NewValue, fSleepMultiplier) \
	RendererCwc_UpdateWrite(hArray, iPosition, NewValue, fSleepMultiplier)
#define Visualizer_UpdateWrite2(hArray, iPositionA, iPositionB, NewValueA, NewValueB, fSleepMultiplier) \
	RendererCwc_UpdateWrite2(hArray, iPositionA, iPositionB, NewValueA, NewValueB, fSleepMultiplier)
#define Visualizer_UpdateWriteMulti(hArray, iStartPosition, Length, aNewValue, fSleepMultiplier) \
	RendererCwc_UpdateWriteMulti(hArray, iStartPosition, Length, aNewValue, fSleepMultiplier)


#define Visualizer_CreatePointer(hArray, iPosition) \
	RendererCwc_CreatePointer(hArray, iPosition)
#define Visualizer_RemovePointer(Pointer) \
	RendererCwc_RemovePointer(Pointer)
#define Visualizer_MovePointer(pPointer, iNewPosition) \
	RendererCwc_MovePointer(pPointer, iNewPosition)

#define Visualizer_SetAlgorithmName(sAlgorithmName) \
	RendererCwc_SetAlgorithmName(sAlgorithmName)
#define Visualizer_ClearReadWriteCounter(hArray) \
	RendererCwc_ClearReadWriteCounter(hArray)

