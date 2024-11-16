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
} Visualizer_Marker;

Visualizer_Marker RendererCwc_AddMarker(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute
);
Visualizer_Marker RendererCwc_AddMarkerWithValue(
	Visualizer_Handle hArray,
	intptr_t iPosition,
	Visualizer_MarkerAttribute Attribute,
	isort_t Value
);
void RendererCwc_RemoveMarker(Visualizer_Marker Marker);
void RendererCwc_MoveMarker(Visualizer_Marker* pMarker, intptr_t iNewPosition);

#define Renderer_Initialize() RendererCwc_Initialize()
#define Renderer_Uninitialize() RendererCwc_Uninitialize()

#define Renderer_AddArray(Size, aArrayState, ValueMin, ValueMax) \
	RendererCwc_AddArray(Size, aArrayState, ValueMin, ValueMax)
#define Renderer_RemoveArray(hArray) RendererCwc_RemoveArray(hArray)
#define Renderer_UpdateArrayState(hArray, aState) RendererCwc_UpdateArrayState(hArray, aState)

#define Renderer_AddMarker(hArray, iPosition, Attribute) \
	RendererCwc_AddMarker(hArray, iPosition, Attribute)
#define Renderer_AddMarkerWithValue(hArray, iPosition, Attribute, Value) \
	RendererCwc_AddMarkerWithValue(hArray, iPosition, Attribute, Value)
#define Renderer_RemoveMarker(Marker) RendererCwc_RemoveMarker(Marker)
#define Renderer_MoveMarker(Marker, iNewPosition) RendererCwc_MoveMarker(Marker, iNewPosition)
