
#include "Sorts.h"
#include "ArrayRenderer.h"

uintptr_t globalN;

int NTQS_isortCompare(const isort_t* a, const isort_t* b) {
	return (*a > *b) - (*a < *b);
}

void LRQS_Partition(isort_t* array, uintptr_t low, uintptr_t high) {

	isort_t pivot;
	uintptr_t left;
	uintptr_t right;

begin:
	pivot = array[low + (high - low + 1) / 2];
	left = low;
	right = high;

	while (left <= right) {
		while (array[left] < pivot) {
			arUpdateRead2(array, globalN, left, right, 62.5);
			++left;
			arUpdatePointer(array, globalN, left, 1, 0.0);

		}
		while (array[right] > pivot) {
			arUpdateRead2(array, globalN, left, right, 62.5);
			--right;
			arUpdatePointer(array, globalN, right, 2, 0.0);
		}

		if (left <= right) {
			arUpdateSwap(array, globalN, left, right, 62.5);
			ISORT_SWAP(array[left], array[right]);
			++left;
			--right;
			arUpdatePointer(array, globalN, left, 1, 0.0);
			arUpdatePointer(array, globalN, right, 2, 0.0);
		}
	}
	arRemovePointer(array, globalN, 0);
	arRemovePointer(array, globalN, 1);
	arRemovePointer(array, globalN, 2);

	// Call tail optimization
	// (prevents O(n) call stack in worst case)

	// Default small to the left partition
	uintptr_t smallLeft = low;
	uintptr_t smallRight = right;
	// Default big to the right partition
	uintptr_t bigLeft = left;
	uintptr_t bigRight = high;
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

void LeftRightQuickSort(isort_t* array, uintptr_t n) {

	if (n < 2) return;
	globalN = n;
	LRQS_Partition(array, 0, n - 1);
}

void NtdllQuickSort(isort_t* array, uintptr_t n) {
	if (n < 2) return;
	qsort(array, n, sizeof(isort_t), NTQS_isortCompare);
}
