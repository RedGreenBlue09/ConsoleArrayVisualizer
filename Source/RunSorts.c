
#include "RunSorts.h"

#include "Utils/Common.h"
#include "Utils/Time.h"
#include "Utils/Random.h"

void InsertionSort(isort_t* array, intptr_t n);

// ShellSort.c

void ShellSortTokuda(Visualizer_Handle arrayHandle, isort_t* array, intptr_t n);

// QuickSort.c

void LeftRightQuickSort(Visualizer_Handle arrayHandle, isort_t* array, intptr_t n);

// MergeSort.c

void IterativeMergeSort(Visualizer_Handle arrayHandle, isort_t* array, intptr_t n);

// HeapSort.c

void BottomUpHeapSort(Visualizer_Handle arrayHandle, isort_t* array, intptr_t n);

sort_info RunSorts_aSortList[128] = {
	{
		"Shellsort (Tokuda's gaps)",
		ShellSortTokuda,
	},
	{
		"Left/Right Quicksort",
		LeftRightQuickSort,
	},
	{
		"Iterative Mergesort",
		IterativeMergeSort,
	},
	{
		"Bottom-up Heapsort",
		BottomUpHeapSort,
	},
};

uintptr_t RunSorts_nSort = static_arrlen(RunSorts_aSortList);

static void Shuffle(Visualizer_Handle hArray, isort_t* aArray, intptr_t Length) {
	// Linear
	for (intptr_t i = 0; i < Length; ++i) {
		Visualizer_UpdateWrite(hArray, i, (isort_t)i, 0.125);
		aArray[i] = (isort_t)i;
	}

	// Fisher-Yates shuffle
	srand64(clock64());
	for (intptr_t i = Length - 1; i >= 1; --i) {
		intptr_t iRandom = (intptr_t)rand64_bounded(i + 1);
		Visualizer_UpdateSwap(hArray, i, iRandom, 1.0);
		swap(&aArray[i], &aArray[iRandom]);
	}
}

void RunSorts_RunSort(
	sort_info* pSortInfo,
	Visualizer_Handle hArray,
	isort_t* aArray,
	intptr_t Length
) {
	Visualizer_ClearReadWriteCounter(hArray);
	Visualizer_SetAlgorithmName("Shuffling ...");
	Shuffle(hArray, aArray, Length);
	Visualizer_SetAlgorithmName("");
	sleep64(1000000);
	Visualizer_ClearReadWriteCounter(hArray);
	Visualizer_SetAlgorithmName(pSortInfo->sName);
	pSortInfo->SortFunction(hArray, aArray, Length);
	sleep64(2000000);
}
