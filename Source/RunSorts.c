
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Time.h"
#include "Utils/Random.h"

void InsertionSort(isort_t* array, intptr_t n);

// ShellSort.c

void ShellSortTokuda(isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

// QuickSort.c

void LeftRightQuickSort(isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

// MergeSort.c

void IterativeMergeSort(isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

// HeapSort.c

void BottomUpHeapSort(isort_t* array, intptr_t n, Visualizer_Handle arrayHandle);

typedef struct {
	char sName[64];
	void (*SortFunction)(isort_t*, intptr_t, Visualizer_Handle);
} SORT_INFO;

SORT_INFO Sorts_aSortList[128] = {
	{
		"ShellSort (Tokuda's gaps)",
		ShellSortTokuda,
	},
	{
		"Left/Right QuickSort",
		LeftRightQuickSort,
	},
	{
		"Iterative MergeSort",
		IterativeMergeSort,
	},
	{
		"Bottom-up HeapSort",
		BottomUpHeapSort,
	},
};

uintptr_t Sorts_nSort = static_arrlen(Sorts_aSortList);
