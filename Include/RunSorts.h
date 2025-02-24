#pragma once

#include "Visualizer.h"

typedef struct {
	char sName[64];
	void (*SortFunction)(Visualizer_Handle, isort_t*, intptr_t);
} sort_info;

extern sort_info RunSorts_aSortList[];
uintptr_t RunSorts_nSort;

void RunSorts_RunSort(
	sort_info* pSortInfo,
	Visualizer_Handle hArray,
	isort_t* aArray,
	intptr_t Length
);
