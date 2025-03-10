
#include <math.h>

#include "Visualizer.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

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
	const double fPi = 0x1.921FB54442D18p1;

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * fPi / (double)Length - (0.5 * fPi)) + 1.0) * fMax * 0.5
		);
		Visualizer_UpdateWrite(hArray, i, Value, 1.0);
		aArray[i] = Value;
	}
}

void VerifySine(
	rand64_state RngState,
	visualizer_array_handle hArray,
	visualizer_int* aArray,
	intptr_t Length
) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(Length, 0.0625, Visualizer_SleepScale_N)
	);

	double fMax = (double)(Length - 1);
	const double fPi = 0x1.921FB54442D18p1;

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * fPi / (double)Length - (0.5 * fPi)) + 1.0) * fMax * 0.5
		);
		if (aArray[i] == Value)
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Correct);
		else
			Visualizer_CreateMarker(hArray, i, Visualizer_MarkerAttribute_Incorrect);
		Visualizer_Sleep(1.0);
	}

	sleep64(2000000);

	for (intptr_t i = 0; i < Length; ++i) {
		visualizer_int Value = (visualizer_int)round(
			(sin((double)i * fPi / (double)Length - (0.5 * fPi)) + 1.0) * fMax * 0.5
		);
		if (aArray[i] == Value)
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Correct });
		else
			Visualizer_RemoveMarker((visualizer_marker) { hArray, i, Visualizer_MarkerAttribute_Incorrect });
	}
}
