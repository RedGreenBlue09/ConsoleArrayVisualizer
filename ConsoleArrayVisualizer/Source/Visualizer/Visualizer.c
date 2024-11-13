
#include <assert.h>
#include <string.h>

#include "Sorts.h"
#include "Visualizer/Visualizer.h"
#include "Visualizer/Renderer/ColumnWindowsConsole.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

#ifndef VISUALIZER_DISABLED

// TODO: More parameter checks

static const uint64_t DefaultDelay = 10000; // microseconds
static bool bInitialized = false;

void Visualizer_Initialize() {
	Renderer_Initialize();
	bInitialized = true;
}

void Visualizer_Uninitialize() {
	bInitialized = false;
	Renderer_Uninitialize();
}

#ifndef VISUALIZER_DISABLE_SLEEP

void Visualizer_Sleep(double fSleepMultiplier) {
	
	sleep64((uint64_t)((double)DefaultDelay * fSleepMultiplier));
	return;
}

#endif
// Array

Visualizer_Handle Visualizer_AddArray(
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
) {
	return Renderer_AddArray(Size, aArrayState, ValueMin, ValueMax);
}

void Visualizer_RemoveArray(Visualizer_Handle hArray) {
	Renderer_RemoveArray(hArray);
}

void Visualizer_UpdateArrayState(Visualizer_Handle hArray, isort_t* aState) {
	Renderer_UpdateArrayState(hArray, aState);
}

/*
void Visualizer_UpdateArray(
	Visualizer_Handle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {
	Renderer_UpdateArray(
		hArray,
		NewSize,
		ValueMin,
		ValueMax
	);
}*/

// Read & Write

void Visualizer_UpdateRead(Visualizer_Handle hArray, intptr_t iPosition, double fSleepMultiplier) {
	Visualizer_Marker Marker = Renderer_AddMarker(hArray, iPosition, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	Renderer_RemoveMarker(Marker);
	return;
}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(Visualizer_Handle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {
	Visualizer_Marker MarkerA = Renderer_AddMarker(hArray, iPositionA, Visualizer_MarkerAttribute_Read);
	Visualizer_Marker MarkerB = Renderer_AddMarker(hArray, iPositionB, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	Renderer_RemoveMarker(MarkerA);
	Renderer_RemoveMarker(MarkerB);
	return;
}

void Visualizer_UpdateReadMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {
	assert(Length <= 16);

	Visualizer_Marker aMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		aMarker[i] = Renderer_AddMarker(hArray, iStartPosition + i, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		Renderer_RemoveMarker(aMarker[i]);
	return;
}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(Visualizer_Handle hArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {
	Visualizer_Marker Marker = Renderer_AddMarkerWithValue(hArray, iPosition, Visualizer_MarkerAttribute_Write, NewValue);
	Visualizer_Sleep(fSleepMultiplier);
	Renderer_RemoveMarker(Marker);
	return;
}

void Visualizer_UpdateWrite2(
	Visualizer_Handle hArray,
	intptr_t iPositionA,
	intptr_t iPositionB,
	isort_t NewValueA,
	isort_t NewValueB,
	double fSleepMultiplier
) {
	Visualizer_Marker MarkerA = Renderer_AddMarkerWithValue(hArray, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	Visualizer_Marker MarkerB = Renderer_AddMarkerWithValue(hArray, iPositionB, Visualizer_MarkerAttribute_Write, NewValueB);
	Visualizer_Sleep(fSleepMultiplier);
	Renderer_RemoveMarker(MarkerA);
	Renderer_RemoveMarker(MarkerB);
	return;
}

void Visualizer_UpdateWriteMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
) {
	assert(Length <= 16);

	Visualizer_Marker aMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		aMarker[i] = Renderer_AddMarkerWithValue(hArray, iStartPosition + i, Visualizer_MarkerAttribute_Write, aNewValue[i]);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		Renderer_RemoveMarker(aMarker[i]);

	free(aMarker);
	return;
}

// Pointer

Visualizer_Pointer Visualizer_CreatePointer(Visualizer_Handle hArray, intptr_t iPosition) {
	return Renderer_AddMarker(hArray, iPosition, Visualizer_MarkerAttribute_Pointer);
}

void Visualizer_RemovePointer(Visualizer_Pointer Pointer) {
	Renderer_RemoveMarker(Pointer);
	return;
}

void Visualizer_MovePointer(Visualizer_Pointer* pPointer, intptr_t iNewPosition) {
	Renderer_MoveMarker(pPointer, iNewPosition);
	return;
}

#endif
