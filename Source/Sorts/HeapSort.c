
#include "Utils/Common.h"
#include "Visualizer.h"

#include "Utils/GuardedMalloc.h"

static void weakHeapSort(visualizer_int* array, usize n) {

	usize i, j, x, y, Gparent;
	usize bitsLength = (n + 7) / 8;
	visualizer_int* bits = malloc_guarded(bitsLength * sizeof(visualizer_int));

	for (i = 0; i < n / 8; ++i)
		bits[i] = 0;

	for (i = n - 1; i > 0; --i) {

		j = i;
		while ((j & 1) == (((bits[(j >> 1) >> 3] >> ((j >> 1) & 7))) & 1))
			j >>= 1;

		Gparent = j >> 1;

		if (array[Gparent] < array[i]) {

			visualizer_int flag = bits[i >> 3];
			flag ^= 1 << (i & 7);

			bits[i >> 3] = flag;

			swap(&array[Gparent], &array[i]);
		}
	}

	for (i = n - 1; i >= 2; --i) {

		swap(&array[0], &array[i]);

		x = 1;
		while (1) {
			y = 2 * x + ((bits[x >> 3] >> (x & 7)) & 1);
			if (y >= i) break;
			x = y;
		}

		while (x > 0) {

			if (array[0] < array[x]) {

				visualizer_int flag = bits[x >> 3];
				flag ^= 1 << (x & 7);

				bits[x >> 3] = flag;

				swap(&array[0], &array[x]);
			}
			x >>= 1;
		}
	}

	swap(&array[0], &array[1]);
	free(bits);
}

void BUHS_SiftDown(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize i, usize end) {

	usize j = i;

	usize left = 2 * j + 1;
	usize right = 2 * j + 2;

	while (left < end) {

		if (right < end) {

			Visualizer_UpdateRead2(iThread, arrayHandle, right, left, 1.0f);
			if ((array[right] > array[left])) {
				j = right;
			} else {
				j = left;
			}

		} else {
			j = left;
		}
		left = 2 * j + 1;
		right = 2 * j + 2;
	}

	while (array[i] > array[j]) {
		Visualizer_UpdateRead2(iThread, arrayHandle, i, j, 1.0f);
		j = (j - 1) / 2;
	}

	while (j > i) {
		Visualizer_UpdateSwap(iThread, arrayHandle, i, j, 1.0f);
		swap(&array[i], &array[j]);
		j = (j - 1) / 2;
	}
}

// Exports:

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n * log(n))
* Extra space                  : No
* Type of sort                 : Comparative - Selection
* Negative integer support     : Yes
*/

void BottomUpHeapSort(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 0.25f, Visualizer_SleepScale_NLogN)
	);

	usize length = n;

	for (isize i = (length - 1) / 2; i >= 0; --i)
		BUHS_SiftDown(iThread, arrayHandle, array, i, length);

	for (usize i = length - 1; i > 0; --i) {
		Visualizer_UpdateSwap(iThread, arrayHandle, 0, i, 1.0f);
		swap(&array[0], &array[i]);
		BUHS_SiftDown(iThread, arrayHandle, array, 0, i);
	}

	return;
}

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n * log(n))
* Extra space                  : O(n)
* Type of sort                 : Comparative - Selection
* Negative integer support     : Yes
*/

void WeakHeapSort(visualizer_array arrayHandle, visualizer_int* array, usize n) {

	if (n < 2) return;
	weakHeapSort(array, n);
	return;

}
