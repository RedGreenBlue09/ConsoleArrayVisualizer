#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCvt_Initialize(intptr_t nMaxArray);
void RendererCvt_Uninitialize();

void RendererCvt_AddArray(
	pool_index ArrayIndex,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void RendererCvt_RemoveArray(
	pool_index ArrayIndex
);
void RendererCvt_UpdateArray(
	pool_index ArrayIndex,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCvt_UpdateItem(
	Visualizer_UpdateRequest* pUpdateRequest
);
