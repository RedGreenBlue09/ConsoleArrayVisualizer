
#include "Sorts.h"
#include "Visualizer/Visualizer.h"

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


double log2fact(double x) {

	double x1 = x + 1.0;
	if (x <= 128.0) {
		double gamma = tgamma(x1);
		return log2(gamma);
	} else {
		double a[8];
		a[0] = x1 * log(x1);
		a[1] = -x1;
		a[2] = -0.5 * log(x1);
		a[3] = 0.5 * log(2 * M_PI);
		a[4] = 1.0 / (12 * x1);
		a[5] = -1.0 / (360 * pow(x1, 3.0));
		a[6] = 1.0 / (1260 * pow(x1, 5.0));
		a[7] = 0.0;
		return (myKahanSum(a, 8) / 0.693147180559945309417);
	}
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

int intcmp(void* a, void* b) {
	return *(int*)a - *(int*)b;
}

int main() {

	//intptr_t N = 512;
	intptr_t N = 512;
	isort_t* aArray = malloc_guarded(N * sizeof(isort_t));
	//sleep64(5000000);

	Visualizer_Initialize();
	rm_handle_t MainArrayHandle = Visualizer_AddArray(N, aArray, 0, (isort_t)N - 1);

	//
	mainShuffle(aArray, N);
	Visualizer_UpdateWriteMulti(MainArrayHandle, 0, N, aArray, 0.0);
	BottomUpHeapSort(aArray, N, MainArrayHandle);

	//
	mainShuffle(aArray, N);
	Visualizer_UpdateWriteMulti(MainArrayHandle, 0, N, aArray, 0.0);
	LeftRightQuickSort(aArray, N, MainArrayHandle);

	//
	mainShuffle(aArray, N);
	Visualizer_UpdateWriteMulti(MainArrayHandle, 0, N, aArray, 0.0);
	ShellSortCiura(aArray, N, MainArrayHandle);

	//
	Visualizer_RemoveArray(0);

	Visualizer_Uninitialize();

	free(aArray);

	return 0;

}
