
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Random.h"

// Sampling exp(-x * x) results in some bias.
// Discrete gaussian kernel (by Tony Lindeberg) solves this
// but I don't understand it so it's hard to properly implement.

const floatptr_t gfSampleLength = 3.0f; // exp(-x * x) from 0 to gfSampleLength

void DistributeGaussian(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (usize i = 0; i < Length; ++i) {
		floatptr_t fX = (floatptr_t)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_UpdateWrite(iThread, hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifyGaussian(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);

	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (usize i = 0; i < Length; ++i) {
		floatptr_t fX = (floatptr_t)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_UpdateCorrectness(iThread, hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifyGaussian(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (usize i = 0; i < Length; ++i) {
		floatptr_t fX = (floatptr_t)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_ClearCorrectness(iThread, hArray, i, aArray[i] == Value);
	}
}

