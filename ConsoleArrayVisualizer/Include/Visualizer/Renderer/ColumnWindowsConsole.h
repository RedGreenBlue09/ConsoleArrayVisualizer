#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCwc_Initialize(intptr_t nMaxArray);
void RendererCwc_Uninitialize();

void RendererCwc_AddArray(
	pool_index ArrayIndex,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void RendererCwc_RemoveArray(
	pool_index ArrayIndex
);
void RendererCwc_UpdateArray(
	pool_index ArrayIndex,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCwc_UpdateItem(
	pool_index ArrayIndex,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
);
