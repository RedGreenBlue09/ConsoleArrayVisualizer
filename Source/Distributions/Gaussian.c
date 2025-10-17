
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Random.h"

// Sampling exp(-x * x) results in some bias.
// Discrete gaussian kernel (by Tony Lindeberg) solves this
// but I don't understand it so it's hard to properly implement.

const floatptr_t gfSampleLength = 3.0f; // exp(-x * x) from 0 to gfSampleLength

void DistributeGaussian(
	randptr_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		floatptr_t fX = (floatptr_t)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_UpdateWrite(hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifyGaussian(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);

	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		floatptr_t fX = (floatptr_t)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifyGaussian(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		floatptr_t fX = (floatptr_t)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}

