#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCwc_Initialize();
void RendererCwc_Uninitialize();

void RendererCwc_AddArray(
	rm_handle_t Handle,
	intptr_t Size
);
void RendererCwc_RemoveArray(
	rm_handle_t Handle
);
void RendererCwc_UpdateArray(
	rm_handle_t Handle,
	intptr_t NewSize,
	isort_t* aNewArrayState,
	bool bVisible,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCwc_UpdateItem(
	rm_handle_t ArrayHandle,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
);
