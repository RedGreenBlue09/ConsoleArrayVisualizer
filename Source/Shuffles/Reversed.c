
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

void ShuffleReversed(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
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
};
