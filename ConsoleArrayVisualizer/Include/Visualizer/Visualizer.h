#pragma once

#include <inttypes.h>
#include <stdbool.h>

//#include "Utils/DynamicArray.h"
#include "Utils/MemoryPool.h"
// #include "Utils/Tree234.h"
#include "Utils/LinkedList.h"
//#include "Utils/ResourceManager.h"

typedef int32_t isort_t;
typedef uint32_t usort_t;

typedef void* Visualizer_Handle;

#define AV_RENDERER_NOUPDATE    (0x00)
#define AV_RENDERER_UPDATEVALUE (0x01)
#define AV_RENDERER_UPDATEATTR  (0x02)

typedef enum {
	Visualizer_MarkerAttribute_Background,
	Visualizer_MarkerAttribute_Normal,
	Visualizer_MarkerAttribute_Read,
	Visualizer_MarkerAttribute_Write,
	Visualizer_MarkerAttribute_Pointer,
	Visualizer_MarkerAttribute_Correct,
	Visualizer_MarkerAttribute_Incorrect,
	Visualizer_MarkerAttribute_EnumCount
} Visualizer_MarkerAttribute;

typedef struct {
	pool_index nMarker;
	pool_index iTailNode;
} Visualizer_MarkerList;

// Array properties struct

typedef struct {

	intptr_t        Size;

	// Array of trees of Visualizer_Marker (used as max heaps)
	// Used to deal with overlapping markers.
	// The i'th position contains a list of markers on i at the same time.
	// tree234**       apMarkerTree;

	// Array of linked lists, each node is a Visualizer_Marker.
	Visualizer_MarkerList* aMarkerList;

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

	void (*UpdateItem)(
		pool_index ArrayIndex,
		intptr_t iPosition,
		uint32_t UpdateRequest,
		isort_t NewValue,
		Visualizer_MarkerAttribute NewAttr
	);

} AV_RENDERER_ENTRY;

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

