
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"

void ShuffleQuickSortAdversary(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	for (intptr_t j = Length - Length % 2 - 2, i = j - 1; i >= 0; i -= 2, j--) {
		Visualizer_UpdateSwap(hArray, i, j, 1.0);
		swap(&aArray[i], &aArray[j]);
	}
}
