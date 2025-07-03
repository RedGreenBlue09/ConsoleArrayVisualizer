
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"

// Sampling exp(-x * x) results in some bias.
// Discrete gaussian kernel (by Tony Lindeberg) solves this
// but I don't understand it so it's hard to properly implement.

const double gfSampleLength = 3.0; // exp(-x * x) from 0 to gfSampleLength

void DistributeGaussian(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	double fMax = (double)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		double fX = (double)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

void VerifyGaussian(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fSampleLength = 4.0;
	double fMax = (double)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		double fX = (double)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0);
	}
}

void UnverifyGaussian(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	double fMax = (double)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		double fX = (double)(Length - 1 - i) * gfSampleLength / fMax;
		visualizer_int Value = (visualizer_int)(fMax * exp(-fX * fX));
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}

