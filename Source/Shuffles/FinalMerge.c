
#include "Visualizer.h"
#include "Utils/Random.h"
#include "Utils/GuardedMalloc.h"

void ShuffleFinalMerge(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	// TODO: Visualizer_AddArray
	visualizer_int* aTemp = calloc_guarded(Length / 2, sizeof(visualizer_int));
	visualizer_array_handle hTemp = Visualizer_AddArray(Length / 2, aTemp, 0, Length - 1);
	intptr_t nOdd = Length / 2;
	intptr_t nEven = Length / 2 + Length % 2;

	for (intptr_t i = 0; i < nOdd; ++i) {
		Visualizer_UpdateRead(hArray, i * 2 + 1, 1.0);
		Visualizer_UpdateWrite(hTemp, i, aArray[i * 2 + 1], 1.0);
		aTemp[i] = aArray[i * 2 + 1];
	}
	for (intptr_t i = 0; i < nEven; ++i) {
		Visualizer_UpdateRead(hArray, i * 2, 1.0);
		Visualizer_UpdateWrite(hArray, i, aArray[i * 2], 1.0);
		aArray[i] = aArray[i * 2];
	}
	for (intptr_t i = 0; i < nOdd; ++i) {
		Visualizer_UpdateRead(hTemp, i, 1.0);
		Visualizer_UpdateWrite(hArray, i + nEven, aTemp[i], 1.0);
		aArray[i + nEven] = aTemp[i];
	}

	Visualizer_UpdateArrayState(hArray, aArray);

	free(aTemp);
}
