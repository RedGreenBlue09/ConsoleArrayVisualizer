
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"

void ShuffleRandom(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	for (usize i = Length - 1; i >= 1; --i) {
		usize iRandom = randptr_bounded(&RngState, i);
		Visualizer_UpdateSwap(iThread, hArray, i, iRandom, 1.0f);
		swap(&aArray[i], &aArray[iRandom]);
	}
}
