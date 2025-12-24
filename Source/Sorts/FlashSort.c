
#include "Utils/Common.h"
#include "Visualizer.h"

#include "Utils/GuardedMalloc.h"

void InsertionSort(visualizer_int* array, usize n);

static void flashSort(visualizer_uint* array, usize n) {

	if (n < 2) return;

	usize M = (n / 5) + 2;

	visualizer_uint min, max;
	usize maxIndex;
	max = array[0];
	min = max;
	maxIndex = 0;

	for (usize i = 1; i < n - 1; i += 2) {
		visualizer_uint smal;
		visualizer_uint big;
		usize bigIndex;

		if (array[i] < array[i + 1]) {
			smal = array[i];
			big = array[i + 1];
			bigIndex = i + 1;
		} else {
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

	if (array[n - 1] < min) {
		min = array[n - 1];
	} else if (array[n - 1] > max) {
		max = array[n - 1];
		maxIndex = n - 1;
	}

	if (max == min) return;

	usize* L = malloc_guarded((M + 1) * sizeof(usize));
	if (L == 0) return;

	for (usize t = 1; t <= M; ++t)
		L[t] = 0;

	usize m1 = M - 1;
	usize range = max - min;

	usize k;
	for (usize h = 0; h < n; ++h) {
		k = (usize)(array[h] - min) * m1 / range + 1;
		L[k] += 1;
	}

	for (k = 2; k <= M; ++k)
		L[k] += L[k - 1];

	swap(&array[maxIndex], &array[0]);

	k = M;
	usize j = 0;
	usize numMoves = 0;

	while (numMoves < n) {

		while (j >= L[k]) {
			++j;
			k = (array[j] - min) * m1 / range + 1;
		}

		visualizer_uint evicted = array[j];

		while (j < L[k]) {

			k = (evicted - min) * m1 / range + 1;
			usize location = L[k] - 1;

			swap(&array[location], &evicted);

			L[k] -= 1;
			++numMoves;
		}
	}

	usize threshold = ((n / M) * 5 / 4) + 1;
	const usize minElements = 24;

	for (k = M - 1; k > 0; --k) {

		usize classSize = L[k + 1] - L[k];
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

void FlashSort(visualizer_int* array, usize n) {

	if (n < 2) return;
	flashSort((visualizer_uint*)array, n);
	return;

}
