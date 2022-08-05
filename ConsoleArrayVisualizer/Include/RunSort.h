#pragma once
#include "Sorts.h"

// RunSort.c

isort_t* rsCreateSortedArray(uintptr_t n);

void rsShuffle(isort_t* array, uintptr_t n);
void rsCheck(isort_t* array, isort_t* input, uintptr_t n);
void rsRunSort(SORT_INFO* si, isort_t* input, uintptr_t n);
