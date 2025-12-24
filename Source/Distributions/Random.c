
// We need double precision for this one to work.
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"

void DistributeRandom(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (isize i = Length - 1; i >= 0; --i) {
		fCurrentMax *= exp2(log2(randfptr(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateWrite(iThread, hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifyRandom(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (isize i = Length - 1; i >= 0; --i) {
		fCurrentMax *= exp2(log2(randfptr(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateCorrectness(iThread, hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifyRandom(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	double fCurrentMax = (double)(Length - 1);
	for (isize i = Length - 1; i >= 0; --i) {
		fCurrentMax *= exp2(log2(randfptr(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_ClearCorrectness(iThread, hArray, i, aArray[i] == Value);
	}
}
