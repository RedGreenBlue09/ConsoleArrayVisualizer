
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"

void ShuffleReversed(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	isize i = 0;
	isize ii = Length - 1;
	while (i < ii) {
		Visualizer_UpdateSwap(iThread, hArray, i, ii, 1.0f);
		swap(&aArray[i], &aArray[ii]);
		++i;
		--ii;
	}
};
