
#include "Sorts.h"

int NTQS_isortCompare(const isort_t* a, const isort_t* b) {
	return (*a > *b) - (*a < *b);
}

void qsort(void* array, size_t n, size_t size, int (*cmp)(const void*, const void*));

void LRQS_Partition(isort_t* array, uintptr_t low, uintptr_t high) {

	isort_t pivot;
	uintptr_t left;
	uintptr_t right;

begin:
	pivot = array[low + (high - low + 1) / 2];
	left = low;
	right = high;

	while (left <= right) {
		while (array[left] < pivot) ++left;
		while (array[right] > pivot) --right;

		if (left <= right) {

			ISORT_SWAP(array[left], array[right]);
			++left;
			--right;
		}
	}

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

	LRQS_Partition(array, 0, n - 1);
}

void NtdllQuickSort(isort_t* array, uintptr_t n) {
	if (n < 2) return;
	qsort(array, n, sizeof(isort_t), NTQS_isortCompare);
}
