
#include <math.h>

#include "Visualizer.h"
#include "Utils/Machine.h"
#include "Utils/Random.h"

void DistributeSquareRoot(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	double fSqrtMax = sqrt((double)(Length - 1));
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)i) * fSqrtMax);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

void VerifySquareRoot(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	visualizer_int ValueMax = (visualizer_int)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)((visualizer_long)i * ValueMax)));
		Visualizer_UpdateCorrectness(hArray, i, aArray[i] == Value, 1.0);
	}
}

void UnverifySquareRoot(
	rand64_state RngState,
	visualizer_array_handle hArray,
	const visualizer_int* aArray,
	intptr_t Length
) {
	visualizer_int ValueMax = (visualizer_int)(Length - 1);
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)((visualizer_long)i * ValueMax)));
		Visualizer_ClearCorrectness(hArray, i, aArray[i] == Value);
	}
}
