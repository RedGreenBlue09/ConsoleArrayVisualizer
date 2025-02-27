
#include "Visualizer.h"

#include <math.h>

double Visualizer_ScaleSleepMultiplier(intptr_t N, double fMultiplier, visualizer_sleep_scale ScaleMode) {
	double fN = (double)N;
	switch (ScaleMode) {
	default:
	case Visualizer_SleepScale_N:
		break;
	case Visualizer_SleepScale_NLogN:
		fN = fN * log2(fN);
		break;
	case Visualizer_SleepScale_NN:
		fN = fN * fN;
		break;
	}
	return fMultiplier * 8.0 / fN;
}
