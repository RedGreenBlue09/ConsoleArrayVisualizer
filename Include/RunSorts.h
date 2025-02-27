#pragma once

#include "Visualizer.h"

typedef struct {
	char sName[64];
	void (*SortFunction)(visualizer_array_handle, visualizer_int*, intptr_t);
} sort_info;

extern sort_info RunSorts_aSortList[];
extern uintptr_t RunSorts_nSort;

void RunSorts_RunSort(
	sort_info* pSortInfo,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
);
