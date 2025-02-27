#pragma once

#include <stdint.h>

typedef int32_t visualizer_int;
typedef uint32_t visualizer_uint;

typedef int64_t visualizer_long;
typedef uint64_t visualizer_ulong;

typedef void* visualizer_array_handle;

typedef uint8_t visualizer_marker_attribute;
#define Visualizer_MarkerAttribute_Pointer 0
#define Visualizer_MarkerAttribute_Read 1
#define Visualizer_MarkerAttribute_Write 2
#define Visualizer_MarkerAttribute_Correct 3
#define Visualizer_MarkerAttribute_Incorrect 4
#define Visualizer_MarkerAttribute_EnumCount 5

typedef uint8_t visualizer_sleep_scale;
#define Visualizer_SleepScale_N 0
#define Visualizer_SleepScale_NLogN 1
#define Visualizer_SleepScale_NN 2

//#define VISUALIZER_DISABLE_SLEEP 1

void Visualizer_Initialize();
void Visualizer_Uninitialize();

// Delays

double Visualizer_ScaleSleepMultiplier(intptr_t N, double fMultiplier, visualizer_sleep_scale ScaleMode);
void Visualizer_SetAlgorithmSleepMultiplier(double fAlgorithmSleepMultiplier);
void Visualizer_SetUserSleepMultiplier(double fUserSleepMultiplier);
void Visualizer_Sleep(double fSleepMultiplier);

// Array

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
