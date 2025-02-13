
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

	//intptr_t N = 64;
	intptr_t N = 1 << 20;
	isort_t* aArray = malloc_guarded(N * sizeof(isort_t));
	//sleep64(5000000);

	Visualizer_Initialize();
	Visualizer_Handle hArray = Visualizer_AddArray(N, aArray, 0, (isort_t)N - 1);

	//
	Visualizer_ClearReadWriteCounter(hArray);
	Visualizer_SetAlgorithmName("Shuffling ...");
	mainShuffle(aArray, N);
	Visualizer_UpdateArrayState(hArray, aArray);
	Visualizer_SetAlgorithmName("Bottom-up Heapsort");
	BottomUpHeapSort(aArray, N, hArray); // FIXME: This has incorrect results
	

	//sleep64(1500000);

	//
	Visualizer_ClearReadWriteCounter(hArray);
	Visualizer_SetAlgorithmName("Shuffling ...");
	mainShuffle(aArray, N);
	Visualizer_UpdateArrayState(hArray, aArray);
	Visualizer_SetAlgorithmName("Left-right Quicksort");
	LeftRightQuickSort(aArray, N, hArray);

	//sleep64(1500000);

	//
	Visualizer_ClearReadWriteCounter(hArray);
	Visualizer_SetAlgorithmName("Shuffling ...");
	mainShuffle(aArray, N);
	Visualizer_UpdateArrayState(hArray, aArray);
	Visualizer_SetAlgorithmName("Shellsort (Ciura's gaps)");
	ShellSortCiura(aArray, N, hArray);

	//
	Visualizer_RemoveArray(hArray);

	Visualizer_Uninitialize();

	free(aArray);

	return 0;

}
