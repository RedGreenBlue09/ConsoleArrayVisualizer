
#include "RunSorts.h"

#include "Utils/Common.h"
#include "Utils/Time.h"
#include "Utils/Random.h"

void InsertionSort(visualizer_int* array, intptr_t n);

// ShellSort.c

void ShellSortTokuda(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);

// QuickSort.c

void LeftRightQuickSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);

// MergeSort.c

void IterativeMergeSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);

// HeapSort.c

void BottomUpHeapSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n);

sort_info RunSorts_aSortList[] = {
	{
		"Shellsort (Tokuda's gaps)",
		ShellSortTokuda,
	},
	{
		"Left/Right Quicksort",
		LeftRightQuickSort,
	},
	{
		"Bottom-up Heapsort",
		BottomUpHeapSort,
	},
};

uintptr_t RunSorts_nSort = static_arrlen(RunSorts_aSortList);

static void Shuffle(visualizer_array_handle hArray, visualizer_int* aArray, intptr_t Length) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);
	// Linear
	for (intptr_t i = 0; i < Length; ++i) {
		Visualizer_UpdateWrite(hArray, i, (visualizer_int)i, 1.0);
		aArray[i] = (visualizer_int)i;
	}

	// Fisher-Yates shuffle
	srand64(clock64());
	for (intptr_t i = Length - 1; i >= 1; --i) {
		intptr_t iRandom = (intptr_t)rand64_bounded(i + 1);
		Visualizer_UpdateSwap(hArray, i, iRandom, 1.0);
		swap(&aArray[i], &aArray[iRandom]);
	}
}

static void Verify(visualizer_array_handle hArray, visualizer_int* aArray, intptr_t Length) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);
	// Linear
	for (intptr_t i = 0; i < Length; ++i) {
		if (aArray[i] == (visualizer_int)i)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}
	sleep64(2000000);
	for (intptr_t i = 0; i < Length; ++i) {
		if (aArray[i] == (visualizer_int)i)
			Visualizer_RemoveMarker((visualizer_marker){ hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker){ hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}

void RunSorts_RunSort(
	sort_info* pSortInfo,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
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
	Verify(hArray, aArray, Length);
}
