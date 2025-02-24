
#include "Utils/Common.h"
#include "Visualizer.h"


static int isortCompare(const isort_t* a, const isort_t* b) {
	return (*a > *b) - (*a < *b);
}

static void partition(isort_t* array, intptr_t low, intptr_t high, Visualizer_Handle arrayHandle) {

	isort_t pivot;
	intptr_t left;
	intptr_t right;

begin:
	pivot = array[low + (high - low + 1) / 2];
	left = low;
	right = high;
	Visualizer_Pointer pointer = Visualizer_CreatePointer(arrayHandle, low + (high - low + 1) / 2);

	while (left <= right) {
		while (array[left] < pivot) {
			Visualizer_UpdateRead2(arrayHandle, left, right, 0.625);
			++left;

		}
		while (array[right] > pivot) {
			Visualizer_UpdateRead2(arrayHandle, left, right, 0.625);
			--right;
		}

		if (left <= right) {
			Visualizer_UpdateWrite2(arrayHandle, left, right, array[right], array[left], 0.625);
			swap(&array[left], &array[right]);
			++left;
			--right;
		}
	}
	Visualizer_RemovePointer(pointer);

	// Call tail optimization
	// (prevents O(n) call stack in worst case)

	// Default small to the left partition
	intptr_t smallLeft = low;
	intptr_t smallRight = right;
	// Default big to the right partition
	intptr_t bigLeft = left;
	intptr_t bigRight = high;
	if ((right - low) > (high - left)) {
		// (right - low) and (high - left) cannot be negative
		// Switch small to right partition
		smallLeft = left;
		smallRight = high;
		// Switch big to left partition
		bigLeft = low;
		bigRight = right;
	}

	if (bigLeft < bigRight) partition(array, bigLeft, bigRight, arrayHandle);
	if (smallLeft < smallRight) {
		low = smallLeft;
		high = smallRight;
		goto begin;
	}
	return;
}

// Exports

/*
* ALGORITHM INFORMATION:
* Time complexity (worst case) : O(n ^ 2)
* Time complexity (avg. case)  : O(n * log2(n))
* Time complexity (best case)  : O(n * log2(n))
* Extra space (worst case)     : O(log2(n))
* Extra space (best case)      : O(log2(n))
* Type of sort                 : Comparative - Exchange
* Negative integer support     : Yes
*/

void LeftRightQuickSort(isort_t* array, intptr_t n, Visualizer_Handle arrayHandle) {

	if (n < 2) return;

	partition(array, 0, n - 1, arrayHandle);

	return;
}
