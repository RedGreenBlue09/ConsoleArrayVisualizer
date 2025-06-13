
#include "Utils/Common.h"
#include "Visualizer.h"


static int isortCompare(const visualizer_int* a, const visualizer_int* b) {
	return (*a > *b) - (*a < *b);
}

static void partition(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t low, intptr_t high) {
	intptr_t left;
	intptr_t right;
	intptr_t pivot;
	visualizer_int pivotValue;

begin:
	left = low;
	right = high;
	pivot = low + (high - low + 1) / 2;
	pivotValue = array[pivot];

	visualizer_marker pointer = Visualizer_CreateMarker(arrayHandle, pivot, Visualizer_MarkerAttribute_Pointer);
	while (left <= right) {
		while (array[left] < pivotValue) {
			Visualizer_UpdateRead(arrayHandle, left, 0.625);
			++left;

		}
		while (array[right] > pivotValue) {
			Visualizer_UpdateRead(arrayHandle, right, 0.625);
			--right;
		}

		if (left < right) {
			Visualizer_UpdateSwap(arrayHandle, left, right, 0.625);
			swap(&array[left], &array[right]);
			++left;
			--right;
		}
	}
	Visualizer_RemoveMarker(pointer);

	// Call tail optimization
	// Is slower but prevents O(n) call stack in worst case

	intptr_t smallLeft;
	intptr_t smallRight;
	intptr_t bigLeft;
	intptr_t bigRight;
	if ((right - low) > (high - left)) {
		smallLeft = left;
		smallRight = high;
		bigLeft = low;
		bigRight = right;
	} else {
		smallLeft = low;
		smallRight = right;
		bigLeft = left;
		bigRight = high;
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
* Time complexity (avg. case)  : O(n * log(n))
* Time complexity (best case)  : O(n * log(n))
* Extra space (worst case)     : O(log(n))
* Extra space (best case)      : O(log(n))
* Type of sort                 : Comparative - Exchange
* Negative integer support     : Yes
*/

void LeftRightQuickSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0, Visualizer_SleepScale_NLogN)
	);

	if (n < 2) return;

	partition(arrayHandle, array, 0, n - 1);

	return;
}
