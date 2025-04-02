
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

uint64_t RunSorts_Second;

void DistributeRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Distribute: Random");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.125, Visualizer_SleepScale_N)
	);

	double fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 0; --i) {
		fCurrentMax *= exp2(log2(randf64(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

void VerifyRandom(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmName("Verify distribute: Random");
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fCurrentMax;
	rand64_state RngStateOriginal = RngState;

	fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(randf64(&RngState)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		if (aArray[i] == Value)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}

	sleep64(RunSorts_Second * 2);

	fCurrentMax = (double)(Length - 1);
	for (intptr_t i = Length - 1; i >= 1; --i) {
		fCurrentMax *= exp2(log2(randf64(&RngStateOriginal)) / (double)(i + 1));
		visualizer_int Value = (visualizer_int)round(fCurrentMax);
		if (aArray[i] == Value)
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}
