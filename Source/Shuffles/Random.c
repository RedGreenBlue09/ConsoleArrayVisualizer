
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"

void ShuffleRandom(
	randptr_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	for (intptr_t i = Length - 1; i >= 1; --i) {
		intptr_t iRandom = (intptr_t)randptr_bounded(&RngState, i);
		Visualizer_UpdateSwap(hArray, i, iRandom, 1.0f);
		swap(&aArray[i], &aArray[iRandom]);
	}
}
