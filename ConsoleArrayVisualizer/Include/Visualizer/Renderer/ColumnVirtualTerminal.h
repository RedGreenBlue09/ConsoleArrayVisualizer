#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCvt_Initialize();
void RendererCvt_Uninitialize();

void RendererCvt_AddArray(intptr_t ArrayId, intptr_t Size);
void RendererCvt_RemoveArray(intptr_t ArrayId);
void RendererCvt_UpdateArray(
	intptr_t ArrayId,
	isort_t NewSize,
	isort_t* aNewArrayState,
	bool bVisible,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCvt_UpdateItem(
	intptr_t ArrayId,
	uintptr_t iPos,
	uint32_t UpdateRequest,
	isort_t NewValue,
	AvAttribute NewAttr
);
