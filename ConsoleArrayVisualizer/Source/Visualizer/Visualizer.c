
#include <assert.h>
#include <string.h>

#include "Sorts.h"
#include "Visualizer/Visualizer.h"
#include "Visualizer/Renderer/ColumnWindowsConsole.h"
#include "Visualizer/Renderer/ColumnVirtualTerminal.h"

#include "Utils/GuardedMalloc.h"
#include "Utils/Time.h"

#ifndef VISUALIZER_DISABLED

// TODO: More parameter checks

static const uint64_t DefaultDelay = 10000; // microseconds
static bool bInitialized = false;

void Visualizer_Initialize() {
	assert(!bInitialized);

	Visualizer_RendererEntry.Initialize();
	bInitialized = true;
}

void Visualizer_Uninitialize() {
	assert(bInitialized);

	bInitialized = false;
	Visualizer_RendererEntry.Uninitialize();
}

#ifndef VISUALIZER_DISABLE_SLEEP

void Visualizer_Sleep(double fSleepMultiplier) {
	assert(bInitialized);
	sleep64((uint64_t)((double)Visualizer_TimeDefaultDelay * fSleepMultiplier));
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
	assert(bInitialized);
	return Visualizer_RendererEntry.AddArray(Size, aArrayState, ValueMin, ValueMax);
}

void Visualizer_RemoveArray(Visualizer_Handle hArray) {
	assert(bInitialized);
	assert(hArray);
	Visualizer_RendererEntry.RemoveArray(Index);
}

void Visualizer_UpdateArray(
	Visualizer_Handle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
) {
	assert(bInitialized);
	assert(hArray);
	Visualizer_RendererEntry.UpdateArray(
		hArray,
		NewSize,
		ValueMin,
		ValueMax
	);
}

// Read & Write

void Visualizer_UpdateRead(Visualizer_Handle hArray, intptr_t iPosition, double fSleepMultiplier) {

	assert(bInitialized);
	assert(hArray);

	Visualizer_Handle hMarker = Visualizer_NewMarker(hArray, iPosition, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarker);
	return;
}

// Update 2 items (used for comparisions).
void Visualizer_UpdateRead2(Visualizer_Handle hArray, intptr_t iPositionA, intptr_t iPositionB, double fSleepMultiplier) {
	assert(bInitialized);
	assert(hArray);

	Visualizer_Handle hMarkerA = Visualizer_NewMarker(hArray, iPositionA, Visualizer_MarkerAttribute_Read);
	Visualizer_Handle hMarkerB = Visualizer_NewMarker(hArray, iPositionB, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarkerA);
	Visualizer_RemoveMarker(hMarkerB);
	return;
}

void Visualizer_UpdateReadMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	double fSleepMultiplier
) {
	assert(bInitialized);
	assert(hArray);
	assert(Length > 16);

	Visualizer_Handle ahMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		ahMarker[i] = Visualizer_NewMarker(hArray, iStartPosition + i, Visualizer_MarkerAttribute_Read);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		Visualizer_RemoveMarker(ahMarker[i]);
	return;
}

// For time precision, the sort will need to do the write(s) by itself.
void Visualizer_UpdateWrite(Visualizer_Handle hArray, intptr_t iPosition, isort_t NewValue, double fSleepMultiplier) {

	assert(bInitialized);
	assert(hArray);

	Visualizer_Handle hMarker = Visualizer_NewMarkerAndValue(hArray, iPosition, Visualizer_MarkerAttribute_Write, NewValue);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarker);
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
	Visualizer_Handle hMarkerA = Visualizer_NewMarkerAndValue(hArray, iPositionA, Visualizer_MarkerAttribute_Write, NewValueA);
	Visualizer_Handle hMarkerB = Visualizer_NewMarkerAndValue(hArray, iPositionB, Visualizer_MarkerAttribute_Write, NewValueB);
	Visualizer_Sleep(fSleepMultiplier);
	Visualizer_RemoveMarker(hMarkerA);
	Visualizer_RemoveMarker(hMarkerB);
	return;
}

void Visualizer_UpdateWriteMulti(
	Visualizer_Handle hArray,
	intptr_t iStartPosition,
	intptr_t Length,
	isort_t* aNewValue,
	double fSleepMultiplier
) {
	assert(bInitialized);
	if (!hArray) return;
	assert(Length > 16);

	Visualizer_Handle ahMarker[16];
	for (intptr_t i = 0; i < Length; ++i)
		ahMarker[i] = Visualizer_NewMarkerAndValue(hArray, iStartPosition + i, Visualizer_MarkerAttribute_Write, aNewValue[i]);
	Visualizer_Sleep(fSleepMultiplier);
	for (intptr_t i = 0; i < Length; ++i)
		Visualizer_RemoveMarker(ahMarker[i]);

	free(ahMarker);
	return;
}

// Pointer

Visualizer_Handle Visualizer_CreatePointer(Visualizer_Handle hArray, intptr_t iPosition) {
	assert(bInitialized);
	if (!hArray) return NULL;
	return Visualizer_NewMarker(hArray, iPosition, Visualizer_MarkerAttribute_Pointer);
}

void Visualizer_RemovePointer(Visualizer_Handle hPointer) {
	assert(bInitialized);
	Visualizer_RemoveMarker(hPointer);
	return;
}

void Visualizer_MovePointer(Visualizer_Handle hPointer, intptr_t iNewPosition) {
	assert(bInitialized);
	Visualizer_MoveMarker(hPointer, iNewPosition);
	return;
}

#endif