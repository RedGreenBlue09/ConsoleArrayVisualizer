#pragma once

#include "Visualizer.h"
#include "Utils/Common.h"

// Sorts.h

typedef struct {
	char sName[64];
	void (*SortFunction)(isort_t*, intptr_t, Visualizer_Handle);
} SORT_INFO;

// TODO: RunSort.c for sorting algorithms,
//       which will always use 0 as primary array id.
// InsertionSort.c

void InsertionSort      (isort_t* array, intptr_t n);

// ShellSort.c

void ShellSortTokuda    (isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);
void ShellSortCiura     (isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

// QuickSort.c

void LeftRightQuickSort (isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

// MergeSort.c

void IterativeMergeSort (isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

// HeapSort.c

void WeakHeapSort       (isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);
void BottomUpHeapSort   (isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

// Distributive Sorts

void PigeonholeSort     (isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);
void FlashSort          (isort_t* array, intptr_t n);

// Sorts.c

extern uintptr_t Sorts_nSort;
extern SORT_INFO Sorts_aSortList[128];
