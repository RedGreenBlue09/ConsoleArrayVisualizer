
// We need double precision for this one to work.
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"

void DistributeRandom(
	randptr_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 0; --i) {
		fCurrentMax *= exp2(log2(randfptr(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifyRandom(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(randfptr(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifyRandom(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(randfptr(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}
