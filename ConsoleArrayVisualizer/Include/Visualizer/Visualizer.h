#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "Utils/Tree234.h"
#include "Utils/DynamicArray.h"
#include "Utils/ResourceManager.h"

typedef int32_t isort_t;
typedef uint32_t usort_t;

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

	intptr_t        Size;

	// Array of trees of AV_UNIQUEMARKER (used as max heaps)
	// Used to deal with overlapping unique markers.
	// The i'th position contains a list of markers
	// pointing to i at the same time.
	tree234**       aptreeUniqueMarkerMap;

} AV_ARRAYPROP;

typedef struct {

	handle_t     hHandle;
	intptr_t     Size;
	isort_t*     aArrayState;
	AvAttribute* aAttribute;

	bool         bVisible;
	isort_t      ValueMin;
	isort_t      ValueMax;

} AV_ARRAYPROP_RENDERER;

typedef struct {

	void (*Initialize)();
	void (*Uninitialize)();

	void (*AddArray)(intptr_t ArrayId, intptr_t Size);
	void (*RemoveArray)(intptr_t ArrayId);
	void (*UpdateArray)(
		intptr_t ArrayId,
		isort_t NewSize,
		isort_t* aNewArrayState,
		bool bVisible,
		isort_t ValueMin,
		isort_t ValueMax
	);

	void (*UpdateItem)(
		intptr_t ArrayId,
		uintptr_t iPos,
		uint32_t UpdateRequest,
		isort_t NewValue,
		AvAttribute NewAttr
	);

} AV_RENDERER_ENTRY;

// Visualizer.c

// Low level renderer functions.
//#define VISUALIZER_DISABLED

#ifndef VISUALIZER_DISABLED

void Visualizer_Initialize();
void Visualizer_Uninitialize();

#define VISUALIZER_DISABLE_SLEEP
#ifdef VISUALIZER_DISABLE_SLEEP

#define Visualizer_Sleep(X) 

#else

void Visualizer_Sleep(double fSleepMultiplier);

#endif

void Visualizer_AddArray(
	intptr_t ArrayId,
	intptr_t Size
);
void Visualizer_RemoveArray(
	intptr_t ArrayId
);
void Visualizer_UpdateArray(
	intptr_t ArrayId,
	isort_t NewSize,
	isort_t* aNewArrayState,
	int32_t bVisible,
	isort_t ValueMin,
	isort_t ValueMax
);

void Visualizer_UpdateRead(
	intptr_t ArrayId,
	intptr_t iPos,
	double fSleepMultiplier
);
void Visualizer_UpdateRead2(
	intptr_t ArrayId,
	intptr_t iPosA,
	intptr_t iPosB,
	double fSleepMultiplier
);
void Visualizer_UpdateWrite(
	intptr_t ArrayId,
	intptr_t iPos,
	isort_t NewValue,
	double fSleepMultiplier
);
void Visualizer_UpdateWrite2(
	intptr_t ArrayId,
	intptr_t iPosA,
	intptr_t iPosB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
);

void Visualizer_UpdatePointer(
	intptr_t ArrayId,
	intptr_t PointerId,
	intptr_t iNewPos
);
void Visualizer_RemovePointer(
	intptr_t ArrayId,
	intptr_t PointerId
);
intptr_t Visualizer_UpdatePointerAuto(
	intptr_t ArrayId,
	intptr_t PointerId,
	intptr_t iNewPos
);
void Visualizer_RemovePointerAuto(
	intptr_t ArrayId,
	intptr_t PointerId
);

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
#define Visualizer_UpdateWrite2(A, B, C, D, E, F)
#define Visualizer_UpdateSwap(A, B, C, D)
#define Visualizer_UpdatePointer(A, B, C)
#define Visualizer_RemovePointer(A, B)
#define Visualizer_UpdatePointerAuto(A, B, C)
#define Visualizer_RemovePointerAuto(A, B)

#endif

/*
 * UpdateRequest:
 *   AV_RENDERER_UPDATEVALUE
 *   AV_RENDERER_UPDATEATTR
 *
 */
