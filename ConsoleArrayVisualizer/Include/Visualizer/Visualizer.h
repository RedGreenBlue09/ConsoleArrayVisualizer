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
	Visualizer_MarkerAttribute_Background,
	Visualizer_MarkerAttribute_Normal,
	Visualizer_MarkerAttribute_Read,
	Visualizer_MarkerAttribute_Write,
	Visualizer_MarkerAttribute_Pointer,
	Visualizer_MarkerAttribute_Correct,
	Visualizer_MarkerAttribute_Incorrect,
	Visualizer_MarkerAttribute_EnumCount
} Visualizer_MarkerAttribute;

// Array properties struct

typedef struct {

	intptr_t        Size;

	// Array of trees of Visualizer_Marker (used as max heaps)
	// Used to deal with overlapping markers.
	// The i'th position contains a list of markers on i at the same time.
	tree234**       aptreeMarkerMap;

} Visualizer_ArrayProp;

typedef struct {

	void (*Initialize)();
	void (*Uninitialize)();

	void (*AddArray)(
		rm_handle_t Handle,
		intptr_t Size,
		isort_t* aArrayState,
		isort_t ValueMin,
		isort_t ValueMax
	);
	void (*RemoveArray)(rm_handle_t Handle);
	void (*UpdateArray)(
		rm_handle_t Handle,
		intptr_t NewSize,
		isort_t ValueMin,
		isort_t ValueMax
	);

	void (*UpdateItem)(
		rm_handle_t ArrayHandle,
		intptr_t iPosition,
		uint32_t UpdateRequest,
		isort_t NewValue,
		Visualizer_MarkerAttribute NewAttr
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

rm_handle_t Visualizer_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void Visualizer_RemoveArray(
	rm_handle_t ArrayHandle
);
void Visualizer_UpdateArray(
	rm_handle_t ArrayHandle,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

void Visualizer_UpdateRead(
	rm_handle_t ArrayHandle,
	intptr_t iPosition,
	double fSleepMultiplier
);
void Visualizer_UpdateRead2(
	rm_handle_t ArrayHandle,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
);
void Visualizer_UpdateWrite(
	rm_handle_t ArrayHandle,
	intptr_t iPosition,
	isort_t NewValue,
	double fSleepMultiplier
);
void Visualizer_UpdateWrite2(
	rm_handle_t ArrayHandle,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
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

