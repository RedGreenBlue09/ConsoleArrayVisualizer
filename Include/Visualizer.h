#pragma once

#include <stdint.h>

#include "Utils/Common.h"
#include "Utils/ThreadPool.h"

typedef int32_t visualizer_int;
typedef uint32_t visualizer_uint;

typedef int64_t visualizer_long;
typedef uint64_t visualizer_ulong;

typedef void* visualizer_array;

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
#define Visualizer_SleepScale_NLogNLogN 2
#define Visualizer_SleepScale_NN 3

//#define VISUALIZER_DISABLE_SLEEP 1

void Visualizer_Initialize(usize ExtraThreadCount);
void Visualizer_Uninitialize();

// Delays

floatptr_t Visualizer_ScaleSleepMultiplier(usize N, floatptr_t fMultiplier, visualizer_sleep_scale ScaleMode);
void Visualizer_SetAlgorithmSleepMultiplier(floatptr_t fAlgorithmSleepMultiplier);
void Visualizer_SetUserSleepMultiplier(floatptr_t fUserSleepMultiplier);
void Visualizer_Sleep(floatptr_t fSleepMultiplier);

// Array

visualizer_array Visualizer_AddArray(
	usize Size,
	visualizer_int* aArrayState,
	visualizer_int ValueMin,
	visualizer_int ValueMax
);
void Visualizer_RemoveArray(visualizer_array hArray);
void Visualizer_UpdateArrayState(visualizer_array hArray, visualizer_int* aState);

typedef struct {
	visualizer_array hArray;
	usize iPosition;
	visualizer_marker_attribute Attribute;
} visualizer_marker;

// Read & Write

void Visualizer_UpdateRead(usize iThread, visualizer_array hArray, usize iPosition, floatptr_t fSleepMultiplier);
void Visualizer_UpdateRead2(usize iThread, visualizer_array hArray, usize iPositionA, usize iPositionB, floatptr_t fSleepMultiplier);
void Visualizer_UpdateReadMulti(usize iThread, visualizer_array hArray, usize iStartPosition, usize Length, floatptr_t fSleepMultiplier);

void Visualizer_UpdateWrite(usize iThread, visualizer_array hArray, usize iPosition, visualizer_int NewValue, floatptr_t fSleepMultiplier);
void Visualizer_UpdateSwap(usize iThread, visualizer_array hArray, usize iPositionA, usize iPositionB, floatptr_t fSleepMultiplier);
void Visualizer_UpdateReadWrite(usize iThread, visualizer_array hArrayA, visualizer_array hArrayB, usize iPositionA, usize iPositionB, floatptr_t fSleepMultiplier);
void Visualizer_UpdateWriteMulti(usize iThread, visualizer_array hArray, usize iStartPosition, usize Length, visualizer_int* aNewValue, floatptr_t fSleepMultiplier);

// Marker

visualizer_marker Visualizer_CreateMarker(usize iThread, visualizer_array hArray, usize iPosition, visualizer_marker_attribute Attribute);
void Visualizer_RemoveMarker(usize iThread, visualizer_marker Marker);
void Visualizer_MoveMarker(usize iThread, visualizer_marker* pMarker, usize iNewPosition);

// Correctness

void Visualizer_UpdateCorrectness(usize iThread, visualizer_array hArray, usize iPosition, bool bCorrect, floatptr_t fSleepMultiplier);
void Visualizer_ClearCorrectness(usize iThread, visualizer_array hArray, usize iPosition, bool bCorrect);

// Other

void Visualizer_SetAlgorithmName(char* sAlgorithmNameArg);
void Visualizer_ClearReadWriteCounter();

// Timer
void Visualizer_StartTimer();
void Visualizer_StopTimer();
void Visualizer_ResetTimer();

// Thread pool

extern thread_pool* Visualizer_pThreadPool;
