
#include "Visualizer.h"
#include "Utils/Random.h"

void DistributeLinear(
	randptr_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);
	for (intptr_t i = 0; i < Length; ++i) {
		Visualizer_UpdateWrite(hArray, i, (visualizer_int)i, 1.0f);
		aArray[i] = (visualizer_int)i;
	}
}

void VerifyLinear(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);
	for (intptr_t i = 0; i < Length; ++i)
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == (visualizer_int)i, 1.0f);
}

void UnverifyLinear(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	for (intptr_t i = 0; i < Length; ++i)
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == (visualizer_int)i);
}
