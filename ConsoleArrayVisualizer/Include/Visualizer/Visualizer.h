#pragma once

#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "Utils/MemoryPool.h"
#include "Utils/LinkedList.h"
#include "Utils/SpinLock.h"

// TODO: Hide internal stuff

typedef int32_t isort_t;
typedef uint32_t usort_t;

typedef void* Visualizer_Handle;

typedef uint8_t Visualizer_MarkerAttribute;
#define Visualizer_MarkerAttribute_Background 0
#define Visualizer_MarkerAttribute_Normal 1
#define Visualizer_MarkerAttribute_Read 2
#define Visualizer_MarkerAttribute_Write 3
#define Visualizer_MarkerAttribute_Pointer 4
#define Visualizer_MarkerAttribute_Correct 5
#define Visualizer_MarkerAttribute_Incorrect 6
#define Visualizer_MarkerAttribute_EnumCount 7

typedef uint8_t Visualizer_UpdateType;
#define Visualizer_UpdateType_NoUpdate 0
#define Visualizer_UpdateType_UpdateValue (1 << 0)
#define Visualizer_UpdateType_UpdateAttr (1 << 1)

typedef struct {
	llist_node Node;
	pool_index iArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_MarkerNode;

typedef struct {
	spinlock Lock;
	bool bUpdated;
	Visualizer_MarkerAttribute Attribute;
	isort_t Value;
} Visualizer_SharedArrayMember;

typedef struct {
	Visualizer_SharedArrayMember Shared;
	Visualizer_MarkerNode* pMarkerListTail; // Linked list
} Visualizer_ArrayMember;

// Array properties struct

typedef struct {

	intptr_t Size;
	Visualizer_ArrayMember* aState;

} Visualizer_ArrayProp;

typedef struct {

	void (*Initialize)(
		intptr_t nMaxArray
	);
	void (*Uninitialize)();

	void (*AddArray)(
		pool_index ArrayIndex,
		intptr_t Size,
		isort_t* aArrayState,
		isort_t ValueMin,
		isort_t ValueMax
	);
	void (*RemoveArray)(
		pool_index ArrayIndex
	);
	void (*UpdateArray)(
		pool_index ArrayIndex,
		intptr_t NewSize,
		isort_t ValueMin,
		isort_t ValueMax
	);

} AV_RENDERER_ENTRY;

// Visualizer.c

#ifndef VISUALIZER_DISABLED

// Initialization

void Visualizer_Initialize();
void Visualizer_Uninitialize();

// Sleep
#define VISUALIZER_DISABLE_SLEEP 1
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
void Visualizer_UpdateArray(
	Visualizer_Handle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

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

// Pointer
Visualizer_Handle Visualizer_CreatePointer(
	Visualizer_Handle hArray,
	intptr_t iPosition
);
void Visualizer_RemovePointer(
	Visualizer_Handle hPointer
);
void Visualizer_MovePointer(
	Visualizer_Handle hPointer,
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

