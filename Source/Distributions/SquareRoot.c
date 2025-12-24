
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Machine.h"
#include "Utils/Random.h"

void DistributeSquareRoot(
	usize iThread,
	visualizer_array hArray,
	visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	floatptr_t fSqrtMax = sqrt((floatptr_t)(Length - 1));
	for (usize i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((floatptr_t)i) * fSqrtMax);
		Visualizer_UpdateWrite(iThread, hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifySquareRoot(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);

	visualizer_int ValueMax = (visualizer_int)(Length - 1);
	for (usize i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((floatptr_t)((visualizer_long)i * ValueMax)));
		Visualizer_UpdateCorrectness(iThread, hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifySquareRoot(
	usize iThread,
	visualizer_array hArray,
	const visualizer_int* aArray,
	usize Length,
	randptr_state RngState
) {
	visualizer_int ValueMax = (visualizer_int)(Length - 1);
	for (usize i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((floatptr_t)((visualizer_long)i * ValueMax)));
		Visualizer_ClearCorrectness(iThread, hArray, i, aArray[i] == Value);
	}
}
