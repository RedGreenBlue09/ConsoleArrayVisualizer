#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCwc_Initialize();
void RendererCwc_Uninitialize();

void RendererCwc_AddArray(intptr_t ArrayId, intptr_t Size);
void RendererCwc_RemoveArray(intptr_t ArrayId);
void RendererCwc_UpdateArray(
	intptr_t ArrayId,
	isort_t NewSize,
	isort_t* aNewArrayState,
	bool bVisible,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCwc_UpdateItem(
	intptr_t ArrayId,
	uintptr_t iPos,
	uint32_t UpdateRequest,
	isort_t NewValue,
	AvAttribute NewAttr
);
