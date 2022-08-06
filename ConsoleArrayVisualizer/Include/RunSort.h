#pragma once
#include "Sorts.h"

// RunSort.c

isort_t* rsCreateSortedArray(intptr_t n);

void rsShuffle(isort_t* array, intptr_t n);
void rsCheck(isort_t* array, isort_t* input, intptr_t n);
void rsRunSort(SORT_INFO* si, isort_t* input, intptr_t n);
