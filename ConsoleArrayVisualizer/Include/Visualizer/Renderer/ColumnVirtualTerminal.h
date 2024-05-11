#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCvt_Initialize();
void RendererCvt_Uninitialize();

void RendererCvt_AddArray(
	Visualizer_ArrayProp* pArrayProp,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void RendererCvt_RemoveArray(
	Visualizer_ArrayProp* pArrayProp
);
void RendererCvt_UpdateArray(
	Visualizer_ArrayProp* pArrayProp,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCvt_UpdateItem(
	Visualizer_ArrayProp* pArrayProp,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
);
