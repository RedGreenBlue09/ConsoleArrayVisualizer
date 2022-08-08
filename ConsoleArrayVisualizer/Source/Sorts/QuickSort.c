
#include "Sorts.h"
#include "Visualizer.h"

intptr_t globalN;

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
	arUpdatePointer(0, 0, low + (high - low + 1) / 2, 0.0);

	while (left <= right) {
		while (array[left] < pivot) {
			arUpdateRead2(0, left, right, 62.5);
			++left;

		}
		while (array[right] > pivot) {
			arUpdateRead2(0, left, right, 62.5);
			--right;
		}

		if (left <= right) {
			arUpdateSwap(0, left, right, 62.5);
			ISORT_SWAP(array[left], array[right]);
			++left;
			--right;
		}
	}
	arRemovePointer(0, 0);

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

	globalN = n;
	arAddArray(0, array, n, (isort_t)n - 1);
	arUpdateArray(0);

	if (n < 2) return;
	LRQS_Partition(array, 0, n - 1);
	arRemoveArray(0);
}

void StdlibQuickSort(isort_t* array, intptr_t n) {
	if (n < 2) return;
	qsort(array, n, sizeof(isort_t), NTQS_isortCompare);
}
