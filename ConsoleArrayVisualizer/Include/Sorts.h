#pragma once

#include "Visualizer/Visualizer.h"

// Sorts.h

typedef struct {
	char sName[64];
	void (*SortFunction)(isort_t*, intptr_t, Visualizer_ArrayHandle);
} SORT_INFO;

// Swap macros
#define ISORT_SWAP(X, Y) {isort_t temp = (X); (X) = (Y); (Y) = temp;}
#define USORT_SWAP(X, Y) {usort_t temp = (X); (X) = (Y); (Y) = temp;}
#define INTPTR_SWAP(X, Y) {intptr_t temp = (X); (X) = (Y); (Y) = temp;}
#define UINTPTR_SWAP(X, Y) {uintptr_t temp = (X); (X) = (Y); (Y) = temp;}
#define PTR_SWAP(X, Y) {void* temp = (X); (X) = (Y); (Y) = temp;}


// TODO: RunSort.c for sorting algorithms,
//       which will always use 0 as primary array id.
// InsertionSort.c

void InsertionSort      (isort_t* array, intptr_t n);

// ShellSort.c

void ShellSortTokuda    (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void ShellSortCiura     (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void ShellSortPrimeMean (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void ShellSort248       (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void ShellSortPigeon    (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void ShellSortSedgewick1986(isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void ShellSortCbrt16    (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void ShellSortCbrt16p1  (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);

// QuickSort.c

void LeftRightQuickSort (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);

// MergeSort.c

void IterativeMergeSort (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);

// HeapSort.c

void WeakHeapSort       (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void BottomUpHeapSort   (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);

// Distributive Sorts

void PigeonholeSort     (isort_t* array, intptr_t n, Visualizer_ArrayHandle arrayHandle);
void FlashSort          (isort_t* array, intptr_t n);

// Sorts.c

extern uintptr_t Sorts_nSort;
extern SORT_INFO Sorts_aSortList[128];
