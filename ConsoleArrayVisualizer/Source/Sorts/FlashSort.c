
#include "Sorts.h"
#include "malloc.h"

void FS_flashSort(usort_t* array, uintptr_t n) {

	if (n < 2) return;

	uintptr_t M = (n / 5) + 2;

	uintptr_t min, max, maxIndex;
	max = array[0];
	min = max;
	maxIndex = 0;

	for (uintptr_t i = 1; i < n - 1; i += 2) {
		usort_t smal;
		usort_t big;
		uintptr_t bigIndex;

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

	if ((uintptr_t)array[n - 1] < min) {

		min = (uintptr_t)array[n - 1];


	}
	else if (array[n - 1] > (usort_t)max) {
		max = (uintptr_t)array[n - 1];
		maxIndex = n - 1;
	}

	if (max == min) return;

	usort_t* L = malloc((M + 1) * sizeof(usort_t));
	if (L == 0) return;

	for (uintptr_t t = 1; t <= M; ++t)
		L[t] = 0;

	uintptr_t m1 = M - 1;
	uintptr_t range = max - min;

	uintptr_t k;
	for (uintptr_t h = 0; h < n; ++h) {
		k = (array[h] - min) * m1 / range + 1;
		L[k] += 1;
	}

	for (k = 2; k <= M; ++k)
		L[k] += L[k - 1];

	USORT_SWAP(array[maxIndex], array[0]);

	k = M;
	uintptr_t j = 0;
	uintptr_t numMoves = 0;

	while (numMoves < n) {

		while (j >= L[k]) {
			++j;
			k = (array[j] - min) * m1 / range + 1;
		}

		usort_t evicted = array[j];

		while (j < L[k]) {

			k = (evicted - min) * m1 / range + 1;
			uintptr_t location = (uintptr_t)L[k] - 1;

			USORT_SWAP(array[location], evicted);

			L[k] -= 1;
			++numMoves;
		}
	}

	uintptr_t threshold = ((n / M) * 5 / 4) + 1;
	const uintptr_t minElements = 24;

	for (k = M - 1; k > 0; --k) {

		uintptr_t classSize = L[k + 1] - L[k];
		if ((classSize > threshold) && (classSize > minElements))
			FS_flashSort(array + L[k], classSize);
	}
	free(L);
	InsertionSort(array, n);

}

// Exports:

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n)
* Extra space                  : O(n)
* Integer only!
*/

void FlashSort(isort_t* array, uintptr_t n) {

	if (n < 2) return;
	FS_flashSort((usort_t*)array, n);
	return;

}
