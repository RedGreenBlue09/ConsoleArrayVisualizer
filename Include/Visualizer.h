#pragma once

#include <stdint.h>

typedef int32_t visualizer_int;
typedef uint32_t visualizer_uint;

typedef int64_t visualizer_long;
typedef uint64_t visualizer_ulong;

typedef void* visualizer_array_handle;

typedef uint8_t visualizer_marker_attribute;
#define visualizer_marker_attribute_Pointer 0
#define visualizer_marker_attribute_Read 1
#define visualizer_marker_attribute_Write 2
#define visualizer_marker_attribute_Correct 3
#define visualizer_marker_attribute_Incorrect 4
#define visualizer_marker_attribute_EnumCount 5

#define VISUALIZER_DISABLE_SLEEP 1

void Visualizer_Initialize();
void Visualizer_Uninitialize();

visualizer_array_handle Visualizer_AddArray(
	intptr_t Size,
	visualizer_int* aArrayState,
	visualizer_int ValueMin,
	visualizer_int ValueMax
);
void Visualizer_RemoveArray(visualizer_array_handle hArray);
void Visualizer_UpdateArrayState(visualizer_array_handle hArray, visualizer_int* aState);

typedef struct {
	visualizer_array_handle hArray;
	intptr_t iPosition;
	visualizer_marker_attribute Attribute;
} visualizer_marker;

// Read

void Visualizer_UpdateRead(
	visualizer_array_handle hArray,
	intptr_t iPosition,
	double fSleepMultiplier
);
void Visualizer_UpdateRead2(
	visualizer_array_handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
);
void Visualizer_UpdateReadMulti(
	visualizer_array_handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
);

// Write

void Visualizer_UpdateWrite(
	visualizer_array_handle hArray,
	intptr_t iPosition,
	visualizer_int NewValue,
	double fSleepMultiplier
);
void Visualizer_UpdateSwap(
	visualizer_array_handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
);
void Visualizer_UpdateReadWrite(
	visualizer_array_handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	double fSleepMultiplier
);
void Visualizer_UpdateWriteMulti(
	visualizer_array_handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	visualizer_int* aNewValue,
	double fSleepMultiplier
);

// Marker

visualizer_marker Visualizer_CreateMarker(
	visualizer_array_handle hArray,
	intptr_t iPosition,
	visualizer_marker_attribute Attribute
);
void Visualizer_RemoveMarker(
	visualizer_marker Marker
);
void Visualizer_MoveMarker(
	visualizer_marker* pMarker,
	intptr_t iNewPosition
);

void Visualizer_SetAlgorithmName(char* sAlgorithmNameArg);
void Visualizer_ClearReadWriteCounter(visualizer_array_handle hArray);
