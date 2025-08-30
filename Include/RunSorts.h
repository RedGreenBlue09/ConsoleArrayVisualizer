#pragma once

#include "Visualizer.h"
#include "Utils/Random.h"

typedef void sort_function(visualizer_array_handle, visualizer_int*, intptr_t);

typedef struct {
	char sName[56];
	sort_function* pSort;
} sort_info;

extern sort_info RunSorts_aSort[];
extern uintptr_t RunSorts_nSort;

typedef void distribute_function(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
typedef void verify_function(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
typedef void unverify_function(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);

typedef struct {
	char sName[56];
	distribute_function* pDistribute;
	verify_function* pVerify;
	unverify_function* pUnverify;
} distribution_info;

extern distribution_info RunSorts_aDistribution[];
extern uintptr_t RunSorts_nDistribution;

typedef void shuffle_function(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);

typedef struct {
	char sName[56];
	shuffle_function* pShuffle;
} shuffle_info;

extern shuffle_info RunSorts_aShuffle[];
extern uintptr_t RunSorts_nShuffle;

void RunSorts_RunSort(
	sort_info* pSort,
	distribution_info* pDistribution,
	shuffle_info* pShuffle,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
);
