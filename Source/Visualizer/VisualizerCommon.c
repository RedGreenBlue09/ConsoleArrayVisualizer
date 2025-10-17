
#include "Visualizer.h"

#include <tgmath.h>

floatptr_t Visualizer_ScaleSleepMultiplier(intptr_t N, floatptr_t fMultiplier, visualizer_sleep_scale ScaleMode) {
	floatptr_t fN = (floatptr_t)N;
	switch (ScaleMode) {
	default:
	case Visualizer_SleepScale_N:
		break;
	case Visualizer_SleepScale_NLogN:
		fN = fN * log2(fN);
		break;
	case Visualizer_SleepScale_NLogNLogN:
		floatptr_t fLogN = log2(fN);
		fN = fN * fLogN * fLogN;
		break;
	case Visualizer_SleepScale_NN:
		fN = fN * fN;
		break;
	}
	return fMultiplier / fN;
}
