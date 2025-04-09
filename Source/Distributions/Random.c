
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"

void DistributeRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 0; --i) {
		fCurrentMax *= exp2(log2(randf64(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

void VerifyRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(randf64(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0);
	}
}

void UnverifyRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(randf64(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}
