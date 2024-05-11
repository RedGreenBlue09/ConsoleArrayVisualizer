#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCvt_Initialize(intptr_t nMaxArray);
void RendererCvt_Uninitialize();

void RendererCvt_AddArray(
	Visualizer_ArrayHandle hArray,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void RendererCvt_RemoveArray(
	Visualizer_ArrayHandle hArray
);
void RendererCvt_UpdateArray(
	Visualizer_ArrayHandle hArray,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCvt_UpdateItem(
	Visualizer_ArrayHandle hArray,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
);
