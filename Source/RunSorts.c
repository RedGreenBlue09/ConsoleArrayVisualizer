
#include "RunSorts.h"

#include <math.h>

#include "Utils/Common.h"
#include "Utils/Machine.h"
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

static void DistributeRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Distribute: Random");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 0; --i) {
		fCurrentMax *= exp2(log2(rand_double(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

static void DistributeLinear(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Distribute: Linear");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);
	for (intptr_t i = 0; i < Length; ++i) {
		Visualizer_UpdateWrite(hArray, i, (visualizer_int)i, 1.0);
		aArray[i] = (visualizer_int)i;
	}
}

static void DistributeSquareRoot(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Distribute: Square root");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	double fSqrtMax = sqrt((double)(Length - 1));
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)i) * fSqrtMax);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

static void DistributeSine(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Distribute: Sine");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	double fMax = (double)(Length - 1);
	const double fPi = 0x1.921FB54442D18p1;

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * fPi / (double)Length - (0.5 * fPi)) + 1.0) * fMax * 0.5
		);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

static void ShuffleRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Shuffle: Random");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	for (intptr_t i = Length - 1; i >= 1; --i) {
		intptr_t iRandom = (intptr_t)rand64_bounded(&RngState, i + 1);
		Visualizer_UpdateSwap(hArray, i, iRandom, 1.0);
		swap(&aArray[i], &aArray[iRandom]);
	}
}

static void ShuffleSorted(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Shuffle: Sorted");
}

static void ShuffleReversed(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Shuffle: Reversed");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	intptr_t i = 0;
	intptr_t ii = Length - 1;
	while (i < ii) {
		Visualizer_UpdateSwap(hArray, i, ii, 1.0);
		swap(&aArray[i], &aArray[ii]);
		++i;
		--ii;
	}
}

static void VerifyRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Verify distribute: Random");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fCurrentMax;
	rand64_state RngStateOriginal = RngState;

	fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(rand_double(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		if (aArray[i] == Value)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}

	sleep64(2000000);

	fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(rand_double(&RngStateOriginal)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		if (aArray[i] == Value)
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}

static void VerifyLinear(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Verify distribute: Linear");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

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
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}

static void VerifySquareRoot(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Verify distribute: Square root");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fSqrtMax = sqrt((double)(Length - 1));
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)i) * fSqrtMax);
		if (aArray[i] == Value)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}

	sleep64(2000000);

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)i) * fSqrtMax);
		if (aArray[i] == Value)
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}

static void VerifySine(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Verify distribute: Sine");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fMax = (double)(Length - 1);
	const double fPi = 0x1.921FB54442D18p1;

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * fPi / (double)Length - (0.5 * fPi)) + 1.0) * fMax * 0.5
		);
		if (aArray[i] == Value)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}

	sleep64(2000000);

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * fPi / (double)Length - (0.5 * fPi)) + 1.0) * fMax * 0.5
		);
		if (aArray[i] == Value)
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}

void RunSorts_RunSort(
	sort_info* pSortInfo,
	visualizer_array_distro ArrayDistro,
	visualizer_shuffle ShuffleType,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	// Shuffle

	Visualizer_ClearReadWriteCounter(hArray);

	rand64_state RngState;
	srand64(&RngState, clock64());
	
	switch (ArrayDistro) {
		default:
		case Visualizer_ArrayDistro_Random:
			DistributeRandom(RngState, hArray, aArray, Length);
			break;
		case Visualizer_ArrayDistro_Linear:
			DistributeLinear(RngState, hArray, aArray, Length);
			break;
		case Visualizer_ArrayDistro_SquareRoot:
			DistributeSquareRoot(RngState, hArray, aArray, Length);
			break;
		case Visualizer_ArrayDistro_Sine:
			DistributeSine(RngState, hArray, aArray, Length);
			break;
	}

	switch (ShuffleType) {
		default:
		case Visualizer_Shuffle_Random:
			ShuffleRandom(RngState, hArray, aArray, Length);
			break;
		case Visualizer_Shuffle_Sorted:
			ShuffleSorted(RngState, hArray, aArray, Length);
			break;
		case Visualizer_Shuffle_Reversed:
			ShuffleReversed(RngState, hArray, aArray, Length);
			break;
	}

	sleep64(1000000);

	// Run the sort

	Visualizer_ClearReadWriteCounter(hArray);
	Visualizer_SetAlgorithmName(pSortInfo->sName);
	pSortInfo->SortFunction(hArray, aArray, Length);

	// Verify

	switch (ArrayDistro) {
		default:
		case Visualizer_ArrayDistro_Random:
			VerifyRandom(RngState, hArray, aArray, Length);
			break;
		case Visualizer_ArrayDistro_Linear:
			VerifyLinear(RngState, hArray, aArray, Length);
			break;
		case Visualizer_ArrayDistro_SquareRoot:
			VerifySquareRoot(RngState, hArray, aArray, Length);
			break;
		case Visualizer_ArrayDistro_Sine:
			VerifySine(RngState, hArray, aArray, Length);
			break;
	}
}
