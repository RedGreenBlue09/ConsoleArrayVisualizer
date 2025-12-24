
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"
#include "Utils/GuardedMalloc.h"

void ShuffleReversedFinalMerge(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	usize nOdd = Length / 2;
	usize nEven = Length / 2 + Length % 2;
	visualizer_int* aTemp = calloc_guarded(nOdd, sizeof(visualizer_int));
	visualizer_array hArrayTemp = Visualizer_AddArray(nOdd, aTemp, 0, (visualizer_int)Length - 1);

	for (usize i = 0; i < nOdd; ++i) {
		Visualizer_UpdateReadWrite(iThread, hArrayTemp, hArray, i, i * 2 + 1, 1.0f);
		aTemp[i] = aArray[i * 2 + 1];
	}
	for (usize i = 0; i < nEven; ++i) {
		Visualizer_UpdateReadWrite(iThread, hArray, hArray, i, i * 2, 1.0f);
		aArray[i] = aArray[i * 2];
	}
	for (usize i = 0; i < nOdd; ++i) {
		Visualizer_UpdateReadWrite(iThread, hArray, hArrayTemp, i + nEven, i, 1.0f);
		aArray[i + nEven] = aTemp[i];
	}
	for (usize i = 0; i < nOdd; ++i) {
		Visualizer_UpdateSwap(iThread, hArray, i, Length - i - 1, 1.0f);
		swap(&aArray[i], &aArray[Length - i - 1]);
	}

	Visualizer_RemoveArray(hArrayTemp);
	free(aTemp);
}
