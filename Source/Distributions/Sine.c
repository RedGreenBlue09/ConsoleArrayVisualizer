
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Random.h"

static const floatptr_t gfPi = (floatptr_t)0x1.921FB54442D18p1;

void DistributeSine(
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
		visualizer_int Value = (visualizer_int)round(
			(sin((floatptr_t)i * gfPi / (floatptr_t)Length - (0.5f * gfPi)) + 1.0f) * fMax * 0.5f
		);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifySine(
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
		visualizer_int Value = (visualizer_int)round(
			(sin((floatptr_t)i * gfPi / (floatptr_t)Length - (0.5f * gfPi)) + 1.0f) * fMax * 0.5f
		);
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifySine(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((floatptr_t)i * gfPi / (floatptr_t)Length - (0.5f * gfPi)) + 1.0f) * fMax * 0.5f
		);
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}
