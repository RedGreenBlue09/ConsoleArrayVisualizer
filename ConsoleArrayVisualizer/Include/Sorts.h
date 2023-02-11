#pragma once

#include <stdint.h>

// Sorts.h

typedef int32_t isort_t;
typedef uint32_t usort_t;

typedef struct {
	char sName[64];
	void (*SortFunction)(isort_t*, intptr_t, intptr_t);
} SORT_INFO;

// Swap macros
#define ISORT_SWAP(X, Y) {isort_t temp = (X); (X) = (Y); (Y) = temp;}
#define USORT_SWAP(X, Y) {usort_t temp = (X); (X) = (Y); (Y) = temp;}
#define INTPTR_SWAP(X, Y) {intptr_t temp = (X); (X) = (Y); (Y) = temp;}
#define UINTPTR_SWAP(X, Y) {uintptr_t temp = (X); (X) = (Y); (Y) = temp;}
#define PTR_SWAP(X, Y) {void* temp = (X); (X) = (Y); (Y) = temp;}

// InsertionSort.c

void InsertionSort      (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);

// ShellSort.c

void ShellSortTokuda    (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void ShellSortCiura     (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void ShellSortPrimeMean (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void ShellSort248       (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void ShellSortPigeon    (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void ShellSortSedgewick1986(isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void ShellSortCbrt16    (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void ShellSortCbrt16p1  (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);

// QuickSort.c

void LeftRightQuickSort (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);

// MergeSort.c

void IterativeMergeSort (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);

// HeapSort.c

void WeakHeapSort       (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void BottomUpHeapSort   (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);

// Distributive Sorts

void PigeonholeSort     (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);
void FlashSort          (isort_t* array, intptr_t n, intptr_t PrimaryArrayId);

// Sorts.c

uintptr_t Sorts_nSort;
SORT_INFO Sorts_aSortList[128];
