
#include "Visualizer.h"
#include "Utils/Random.h"

void DistributeLinear(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);
	for (usize i = 0; i < Length; ++i) {
		Visualizer_UpdateWrite(iThread, hArray, i, (visualizer_int)i, 1.0f);
		aArray[i] = (visualizer_int)i;
	}
}

void VerifyLinear(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);
	for (usize i = 0; i < Length; ++i)
		Visualizer_UpdateCorrectness(iThread, hArray, i, aArray[i] == (visualizer_int)i, 1.0f);
}

void UnverifyLinear(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	for (usize i = 0; i < Length; ++i)
		Visualizer_ClearCorrectness(iThread, hArray, i, aArray[i] == (visualizer_int)i);
}
