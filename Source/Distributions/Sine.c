
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"

static const double gfPi = 0x1.921FB54442D18p1;

void DistributeSine(
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
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * gfPi / (double)Length - (0.5 * gfPi)) + 1.0) * fMax * 0.5
		);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

void VerifySine(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fMax = (double)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * gfPi / (double)Length - (0.5 * gfPi)) + 1.0) * fMax * 0.5
		);
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0);
	}
}

void UnverifySine(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	double fMax = (double)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * gfPi / (double)Length - (0.5 * gfPi)) + 1.0) * fMax * 0.5
		);
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}
