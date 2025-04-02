
#include "Visualizer.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

uint64_t RunSorts_Second;

void DistributeLinear(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);
	for (intptr_t i = 0; i < Length; ++i) {
		Visualizer_UpdateWrite(hArray, i, (visualizer_int)i, 1.0);
		aArray[i] = (visualizer_int)i;
	}
}

void VerifyLinear(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	for (intptr_t i = 0; i < Length; ++i) {
		if (aArray[i] == (visualizer_int)i)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}

	sleep64(RunSorts_Second * 2);

	for (intptr_t i = 0; i < Length; ++i) {
		if (aArray[i] == (visualizer_int)i)
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}
