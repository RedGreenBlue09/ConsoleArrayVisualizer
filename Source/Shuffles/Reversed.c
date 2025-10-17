
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"

void ShuffleReversed(
	randptr_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	intptr_t i = 0;
	intptr_t ii = Length - 1;
	while (i < ii) {
		Visualizer_UpdateSwap(hArray, i, ii, 1.0f);
		swap(&aArray[i], &aArray[ii]);
		++i;
		--ii;
	}
};
