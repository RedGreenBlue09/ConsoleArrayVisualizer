
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Random.h"

static const floatptr_t gfPi = (floatptr_t)0x1.921FB54442D18p1;

void DistributeSine(
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
		visualizer_int Value = (visualizer_int)round(
			(sin((floatptr_t)i * gfPi / (floatptr_t)Length - (0.5f * gfPi)) + 1.0f) * fMax * 0.5f
		);
		Visualizer_UpdateWrite(iThread, hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifySine(
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
		visualizer_int Value = (visualizer_int)round(
			(sin((floatptr_t)i * gfPi / (floatptr_t)Length - (0.5f * gfPi)) + 1.0f) * fMax * 0.5f
		);
		Visualizer_UpdateCorrectness(iThread, hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifySine(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	floatptr_t fMax = (floatptr_t)(Length - 1);
	for (usize i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((floatptr_t)i * gfPi / (floatptr_t)Length - (0.5f * gfPi)) + 1.0f) * fMax * 0.5f
		);
		Visualizer_ClearCorrectness(iThread, hArray, i, aArray[i] == Value);
	}
}
