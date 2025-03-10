
#include "Visualizer.h"
#include "Utils/Common.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

void ShuffleRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	for (intptr_t i = Length - 1; i >= 1; --i) {
		intptr_t iRandom = (intptr_t)rand64_bounded(&RngState, i + 1);
		Visualizer_UpdateSwap(hArray, i, iRandom, 1.0);
		swap(&aArray[i], &aArray[iRandom]);
	}
}
