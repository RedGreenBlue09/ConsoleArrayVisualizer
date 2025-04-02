
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

uint64_t RunSorts_Second;

void DistributeSquareRoot(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Distribute: Square root");
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
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Verify distribute: Square root");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fSqrtMax = sqrt((double)(Length - 1));
	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)i) * fSqrtMax);
		if (aArray[i] == Value)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}

	sleep64(RunSorts_Second * 2);

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(sqrt((double)i) * fSqrtMax);
		if (aArray[i] == Value)
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}
