
#include "Utils/Common.h"
#include "Visualizer.h"

#include "Utils/GuardedMalloc.h"

void InsertionSort(visualizer_int* array, intptr_t n);

static void flashSort(visualizer_uint* array, intptr_t n) {

	if (n < 2) return;

	intptr_t M = (n / 5) + 2;

	visualizer_uint min, max;
	intptr_t maxIndex;
	max = array[0];
	min = max;
	maxIndex = 0;

	for (intptr_t i = 1; i < n - 1; i += 2) {
		visualizer_uint smal;
		visualizer_uint big;
		intptr_t bigIndex;

		if (array[i] < array[i + 1]) {
			smal = array[i];
			big = array[i + 1];
			bigIndex = i + 1;
		}
		else {
			big = array[i];
			bigIndex = i;
			smal = array[i + 1];
		}
		if (big > max) {
			max = big;
			maxIndex = bigIndex;
		}
		if (smal < min) min = smal;
	}

	if ((intptr_t)array[n - 1] < min) {

		min = (intptr_t)array[n - 1];


	}
	else if (array[n - 1] > (visualizer_uint)max) {
		max = (intptr_t)array[n - 1];
		maxIndex = n - 1;
	}

	if (max == min) return;

	visualizer_uint* L = malloc_guarded((M + 1) * sizeof(visualizer_uint));
	if (L == 0) return;

	for (intptr_t t = 1; t <= M; ++t)
		L[t] = 0;

	intptr_t m1 = M - 1;
	intptr_t range = max - min;

	intptr_t k;
	for (intptr_t h = 0; h < n; ++h) {
		k = (array[h] - min) * m1 / range + 1;
		L[k] += 1;
	}

	for (k = 2; k <= M; ++k)
		L[k] += L[k - 1];

	swap(&array[maxIndex], &array[0]);

	k = M;
	uintptr_t j = 0;
	intptr_t numMoves = 0;

	while (numMoves < n) {

		while (j >= L[k]) {
			++j;
			k = (array[j] - min) * m1 / range + 1;
		}

		visualizer_uint evicted = array[j];

		while (j < L[k]) {

			k = (evicted - min) * m1 / range + 1;
			intptr_t location = (intptr_t)L[k] - 1;

			swap(&array[location], &evicted);

			L[k] -= 1;
			++numMoves;
		}
	}

	intptr_t threshold = ((n / M) * 5 / 4) + 1;
	const intptr_t minElements = 24;

	for (k = M - 1; k > 0; --k) {

		intptr_t classSize = L[k + 1] - L[k];
		if ((classSize > threshold) && (classSize > minElements))
			flashSort(array + L[k], classSize);
	}
	free(L);
	InsertionSort((visualizer_int*)array, n);

}

// Exports:

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n)
* Extra space                  : O(n)
* Integer only!
*/

void FlashSort(visualizer_int* array, intptr_t n) {

	if (n < 2) return;
	flashSort((visualizer_uint*)array, n);
	return;

}
