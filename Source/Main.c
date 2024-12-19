
#include "Sorts.h"

#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>
#include "Utils/Time.h"
#include "Utils/Random.h"
#include "Utils/GuardedMalloc.h"

double myKahanSum(double* ax, size_t n) {

	double sum = 0.0;
	double c = 0.0;
	for (size_t i = 0; i < n; ++i) {
		double y = ax[i] - c;
		double t = sum + y;
		c = (t - sum) - y;
		sum = t;
	}
	return sum;

}

void mainShuffle(isort_t* aArray, intptr_t N) {

	// Dumb shuffle for testing

	srand64(clock64());
	for (intptr_t i = 0; i < N; ++i) {

		isort_t X = (isort_t)(((rand64() >> 32ull) * (int64_t)N) >> 32ull);
		aArray[i] = X;

	}

	return;

}

int main() {

	//intptr_t N = 16384;
	intptr_t N = 1 << 24;
	isort_t* aArray = malloc_guarded(N * sizeof(isort_t));
	//sleep64(5000000);

	Visualizer_Initialize();
	Visualizer_Handle MainArrayHandle = Visualizer_AddArray(N, aArray, 0, (isort_t)N - 1);

	//
	mainShuffle(aArray, N);
	Visualizer_UpdateArrayState(MainArrayHandle, aArray);
	BottomUpHeapSort(aArray, N, MainArrayHandle); // This has incorrect results
	

	//sleep64(1500000);

	//
	mainShuffle(aArray, N);
	Visualizer_UpdateArrayState(MainArrayHandle, aArray);
	LeftRightQuickSort(aArray, N, MainArrayHandle);

	//sleep64(1500000);

	//
	mainShuffle(aArray, N);
	Visualizer_UpdateArrayState(MainArrayHandle, aArray);
	ShellSortCiura(aArray, N, MainArrayHandle);

	//
	Visualizer_RemoveArray(MainArrayHandle);

	Visualizer_Uninitialize();

	free(aArray);

	return 0;

}
