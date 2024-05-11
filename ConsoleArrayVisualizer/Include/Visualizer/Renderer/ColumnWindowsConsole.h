#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCwc_Initialize();
void RendererCwc_Uninitialize();

void RendererCwc_AddArray(
	Visualizer_ArrayProp* pArrayProp,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void RendererCwc_RemoveArray(
	Visualizer_ArrayProp* pArrayProp
);
void RendererCwc_UpdateArray(
	Visualizer_ArrayProp* pArrayProp,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCwc_UpdateItem(
	Visualizer_ArrayProp* pArrayProp,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
);
