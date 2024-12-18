#pragma once

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
/*
void RendererCwc_UpdateArray(
	Visualizer_Handle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);
*/

typedef struct {
	Visualizer_Handle hArray;
	intptr_t iPosition;
	Visualizer_MarkerAttribute Attribute;
} Visualizer_Pointer;

#define Visualizer_Initialize() RendererCwc_Initialize()
#define Visualizer_Uninitialize() RendererCwc_Uninitialize()

#define Visualizer_AddArray(Size, aArrayState, ValueMin, ValueMax) \
	RendererCwc_AddArray(Size, aArrayState, ValueMin, ValueMax)
#define Visualizer_RemoveArray(hArray) RendererCwc_RemoveArray(hArray)
#define Visualizer_UpdateArrayState(hArray, aState) RendererCwc_UpdateArrayState(hArray, aState)

#define Visualizer_AddMarker(hArray, iPosition, Attribute) \
	RendererCwc_AddMarker(hArray, iPosition, Attribute)
#define Visualizer_AddMarkerWithValue(hArray, iPosition, Attribute, Value) \
	RendererCwc_AddMarkerWithValue(hArray, iPosition, Attribute, Value)
#define Visualizer_RemoveMarker(Marker) RendererCwc_RemoveMarker(Marker)
#define Visualizer_MoveMarker(Marker, iNewPosition) RendererCwc_MoveMarker(Marker, iNewPosition)
