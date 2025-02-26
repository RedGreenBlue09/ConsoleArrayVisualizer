
#include "Utils/Common.h"
#include "Visualizer.h"


static int isortCompare(const isort_t* a, const isort_t* b) {
	return (*a > *b) - (*a < *b);
}

static void partition(Visualizer_Handle arrayHandle, isort_t* array, intptr_t low, intptr_t high) {

	intptr_t pivot;
	isort_t pivotValue;
	intptr_t left;
	intptr_t right;

begin:
	pivot = low + (high - low + 1) / 2;
	pivotValue = array[pivot];
	left = low;
	right = high;

	while (left <= right) {
		// Visualizer_UpdateRead is more accurate here
		// but in the real world, we can't just cache the pivot value
		while (array[left] < pivotValue) {
			Visualizer_UpdateRead2(arrayHandle, left, pivot, 0.625);
			++left;

		}
		while (array[right] > pivotValue) {
			Visualizer_UpdateRead2(arrayHandle, right, pivot, 0.625);
			--right;
		}

		if (left <= right) {
			Visualizer_UpdateSwap(arrayHandle, left, right, 0.625);
			swap(&array[left], &array[right]);
			++left;
			--right;
		}
	}

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

	if (bigLeft < bigRight) partition(arrayHandle, array, bigLeft, bigRight);
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

void LeftRightQuickSort(Visualizer_Handle arrayHandle, isort_t* array, intptr_t n) {

	if (n < 2) return;

	partition(arrayHandle, array, 0, n - 1);

	return;
}
