#pragma once

#include "Visualizer.h"
#include "Utils/Random.h"

typedef struct {
	char sName[56];
	void (*Sort)(visualizer_array_handle, visualizer_int*, intptr_t);
} sort_info;

extern sort_info RunSorts_aSort[];
extern uintptr_t RunSorts_nSort;

typedef struct {
	char sName[56];
	void (*Distribute)(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
	void (*Verify)(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
	void (*Unverify)(rand64_state, visualizer_array_handle, const visualizer_int*, intptr_t);
} distribution_info;

extern distribution_info RunSorts_aDistribution[];
extern uintptr_t RunSorts_nDistribution;

typedef struct {
	char sName[56];
	void (*Shuffle)(rand64_state, visualizer_array_handle, visualizer_int*, intptr_t);
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
