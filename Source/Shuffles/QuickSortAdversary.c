
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"

void ShuffleQuickSortAdversary(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	for (isize j = Length - Length % 2 - 2, i = j - 1; i >= 0; i -= 2, j--) {
		Visualizer_UpdateSwap(iThread, hArray, i, j, 1.0f);
		swap(&aArray[i], &aArray[j]);
	}
}
