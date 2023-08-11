
#include "Sorts.h"
#include "Visualizer/Visualizer.h"

intptr_t GlobalPrimaryArrayId;

int NTQS_isortCompare(const isort_t* a, const isort_t* b) {
	return (*a > *b) - (*a < *b);
}

void LRQS_Partition(isort_t* array, intptr_t low, intptr_t high) {

	isort_t pivot;
	intptr_t left;
	intptr_t right;

begin:
	pivot = array[low + (high - low + 1) / 2];
	left = low;
	right = high;
	Visualizer_UpdatePointer(0, 0, low + (high - low + 1) / 2);

	while (left <= right) {
		while (array[left] < pivot) {
			Visualizer_UpdateRead2(0, left, right, 0.625);
			++left;

		}
		while (array[right] > pivot) {
			Visualizer_UpdateRead2(0, left, right, 0.625);
			--right;
		}

		if (left <= right) {
			Visualizer_UpdateWrite2(0, left, right, array[right], array[left], 0.625);
			ISORT_SWAP(array[left], array[right]);
			++left;
			--right;
		}
	}
	Visualizer_RemovePointer(0, 0);

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

	if (bigLeft < bigRight) LRQS_Partition(array, bigLeft, bigRight);
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

void LeftRightQuickSort(isort_t* array, intptr_t n) {

	if (n < 2) return;
	LRQS_Partition(array, 0, n - 1);

	return;
}
