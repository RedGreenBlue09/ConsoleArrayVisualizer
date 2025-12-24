#pragma once

#include "Visualizer.h"
#include "Utils/Random.h"

typedef void sort_function(usize, visualizer_array, visualizer_int*, usize);

typedef struct {
	char sName[56];
	sort_function* pSort;
} sort_info;

extern sort_info RunSorts_aSort[];
extern usize RunSorts_nSort;

typedef void distribute_function(usize, visualizer_array, visualizer_int*, usize, randptr_state);
typedef void verify_function(usize, visualizer_array, const visualizer_int*, usize, randptr_state);
typedef void unverify_function(usize, visualizer_array, const visualizer_int*, usize, randptr_state);

typedef struct {
	char sName[56];
	distribute_function* pDistribute;
	verify_function* pVerify;
	unverify_function* pUnverify;
} distribution_info;

extern distribution_info RunSorts_aDistribution[];
extern usize RunSorts_nDistribution;

typedef void shuffle_function(usize, visualizer_array, visualizer_int*, usize, randptr_state);

typedef struct {
	char sName[56];
	shuffle_function* pShuffle;
} shuffle_info;

extern shuffle_info RunSorts_aShuffle[];
extern usize RunSorts_nShuffle;

void RunSorts_RunSort(
	sort_info* pSort,
	distribution_info* pDistribution,
	shuffle_info* pShuffle,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length
);
