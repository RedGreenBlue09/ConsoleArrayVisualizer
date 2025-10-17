
#include <tgmath.h>

#include "Visualizer.h"
#include "Utils/Machine.h"
#include "Utils/Random.h"

void DistributeSquareRoot(
	randptr_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125f, Visualizer_SleepScale_N)
	);

	floatptr_t fSqrtMax = sqrt((floatptr_t)(Length - 1));
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((floatptr_t)i) * fSqrtMax);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0f);
		aArray[i] = Value;
	}
}

void VerifySquareRoot(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625f, Visualizer_SleepScale_N)
	);

	visualizer_int ValueMax = (visualizer_int)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((floatptr_t)((visualizer_long)i * ValueMax)));
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0f);
	}
}

void UnverifySquareRoot(
	randptr_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	visualizer_int ValueMax = (visualizer_int)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((floatptr_t)((visualizer_long)i * ValueMax)));
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}
