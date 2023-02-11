
#include "Sorts.h"
#include "Visualizer.h"

#include "malloc.h"

intptr_t GlobalPrimaryArrayId;

void WHS_weakHeapSort(isort_t* array, intptr_t n) {

	intptr_t i, j, x, y, Gparent;
	intptr_t bitsLength = (n + 7) / 8;
	isort_t* bits = malloc(bitsLength * sizeof(isort_t));

	for (i = 0; i < n / 8; ++i)
		bits[i] = 0;

	for (i = n - 1; i > 0; --i) {

		j = i;
		while ((j & 1) == (((bits[(j >> 1) >> 3] >> ((j >> 1) & 7))) & 1))
			j >>= 1;

		Gparent = j >> 1;

		if (array[Gparent] < array[i]) {

			isort_t flag = bits[i >> 3];
			flag ^= 1 << (i & 7);

			bits[i >> 3] = flag;

			ISORT_SWAP(array[Gparent], array[i]);
		}
	}

	for (i = n - 1; i >= 2; --i) {

		ISORT_SWAP(array[0], array[i]);

		x = 1;
		while (1) {
			y = 2 * x + ((bits[x >> 3] >> (x & 7)) & 1);
			if (y >= i) break;
			x = y;
		}

		while (x > 0) {

			if (array[0] < array[x]) {

				isort_t flag = bits[x >> 3];
				flag ^= 1 << (x & 7);

				bits[x >> 3] = flag;

				ISORT_SWAP(array[0], array[x]);
			}
			x >>= 1;
		}
	}

	ISORT_SWAP(array[0], array[1]);
	free(bits);
}

void BUHS_SiftDown(isort_t* array, intptr_t i, intptr_t end) {

	intptr_t j = i;

	intptr_t left = 2 * j + 1;
	intptr_t right = 2 * j + 2;
	//arAddPointer(array, globalN, j, 0, 0.0);
	//arAddPointer(array, globalN, left, 1, 0.0);
	//arAddPointer(array, globalN, right, 2, 0.0);

	while (left < end) {
		Visualizer_UpdatePointer(GlobalPrimaryArrayId, 1, left, 0.0);

		if (right < end) {
			Visualizer_UpdatePointer(GlobalPrimaryArrayId, 2, right, 0.0);

			Visualizer_UpdateRead2(GlobalPrimaryArrayId, right, left, 32.0);
			if ((array[right] > array[left])) {
				j = right;
			} else {
				j = left;
			}
			Visualizer_UpdatePointer(GlobalPrimaryArrayId, 0, j, 0.0);

		} else {
			j = left;
			Visualizer_UpdatePointer(GlobalPrimaryArrayId, 0, j, 0.0);
		}
		left = 2 * j + 1;
		right = 2 * j + 2;
	}

	while (array[i] > array[j]) {
		Visualizer_UpdatePointer(GlobalPrimaryArrayId, 0, j, 0.0);
		Visualizer_UpdateRead2(GlobalPrimaryArrayId, i, j, 32.0);
		j = (j - 1) / 2;
	}

	while (j > i) {
		Visualizer_UpdatePointer(GlobalPrimaryArrayId, 0, j, 0.0);
		Visualizer_UpdateSwap(GlobalPrimaryArrayId, i, j, 32.0);
		ISORT_SWAP(array[i], array[j]);
		j = (j - 1) / 2;
	}
	Visualizer_RemovePointer(GlobalPrimaryArrayId, 0);
	Visualizer_RemovePointer(GlobalPrimaryArrayId, 1);
	Visualizer_RemovePointer(GlobalPrimaryArrayId, 2);
}

// Exports:

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n * log2(n))
* Extra space                  : No
* Type of sort                 : Comparative - Selection
* Negative integer support     : Yes
*/

void BottomUpHeapSort(isort_t* array, intptr_t n, intptr_t PrimaryArrayId) {

	GlobalPrimaryArrayId = PrimaryArrayId;

	Visualizer_AddArray(GlobalPrimaryArrayId, array, n);
	Visualizer_UpdateArray(GlobalPrimaryArrayId, TRUE, 0, (isort_t)n - 1);

	intptr_t length = n;

	for (intptr_t i = (length - 1) / 2; i >= 0; --i)
		BUHS_SiftDown(array, i, length);

	for (intptr_t i = length - 1; i > 0; --i) {
		Visualizer_UpdateSwap(GlobalPrimaryArrayId, 0, i, 32.0);
		ISORT_SWAP(array[0], array[i]);
		BUHS_SiftDown(array, 0, i);
	}

	return;
}

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n * log2(n))
* Extra space                  : O(n)
* Type of sort                 : Comparative - Selection
* Negative integer support     : Yes
*/

void WeakHeapSort(isort_t* array, intptr_t n, intptr_t PrimaryArrayId) {

	if (n < 2) return;
	WHS_weakHeapSort(array, n);
	return;

}