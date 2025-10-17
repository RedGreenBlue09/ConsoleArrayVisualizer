
#include "Visualizer.h"
#include "Utils/Random.h"
#include "Utils/GuardedMalloc.h"

void ShuffleFinalMerge(
	randptr_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	intptr_t nOdd = Length / 2;
	intptr_t nEven = Length / 2 + Length % 2;
	visualizer_int* aTemp = calloc_guarded(nOdd, sizeof(visualizer_int));
	visualizer_array_handle hTemp = Visualizer_AddArray(nOdd, aTemp, 0, (visualizer_int)Length - 1);

	for (intptr_t i = 0; i < nOdd; ++i) {
		Visualizer_UpdateReadWrite(hTemp, hArray, i, i * 2 + 1, 1.0f);
		aTemp[i] = aArray[i * 2 + 1];
	}
	for (intptr_t i = 0; i < nEven; ++i) {
		Visualizer_UpdateReadWrite(hArray, hArray, i, i * 2, 1.0f);
		aArray[i] = aArray[i * 2];
	}
	for (intptr_t i = 0; i < nOdd; ++i) {
		Visualizer_UpdateReadWrite(hArray, hTemp, i + nEven, i, 1.0f);
		aArray[i + nEven] = aTemp[i];
	}

	Visualizer_UpdateArrayState(hArray, aArray);

	free(aTemp);
}
