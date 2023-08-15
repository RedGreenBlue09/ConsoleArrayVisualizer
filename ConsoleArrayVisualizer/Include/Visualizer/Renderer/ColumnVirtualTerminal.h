#pragma once

#include "Visualizer/Visualizer.h"

// ColumnWindowsConsole.c

void RendererCvt_Initialize();
void RendererCvt_Uninitialize();

void RendererCvt_AddArray(
	rm_handle_t Handle,
	intptr_t Size,
	isort_t* aArrayState,
	isort_t ValueMin,
	isort_t ValueMax
);
void RendererCvt_RemoveArray(
	rm_handle_t Handle
);
void RendererCvt_UpdateArray(
	rm_handle_t Handle,
	intptr_t NewSize,
	isort_t ValueMin,
	isort_t ValueMax
);

void RendererCvt_UpdateItem(
	rm_handle_t ArrayHandle,
	intptr_t iPosition,
	uint32_t UpdateRequest,
	isort_t NewValue,
	Visualizer_MarkerAttribute NewAttr
);
