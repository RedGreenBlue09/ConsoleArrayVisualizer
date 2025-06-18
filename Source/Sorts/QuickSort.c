
#include "Utils/Common.h"
#include "Visualizer.h"

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
		Visualizer_UpdateRead(arrayHandle, left, 1.0);
		while (array[left] < pivotValue) {
			++left;
			Visualizer_UpdateRead(arrayHandle, left, 1.0);

		}
		Visualizer_UpdateRead(arrayHandle, right, 1.0);
		while (array[right] > pivotValue) {
			--right;
			Visualizer_UpdateRead(arrayHandle, right, 1.0);
		}

		if (left <= right) {
			Visualizer_UpdateSwap(arrayHandle, left, right, 1.0);
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

	if (smallLeft < smallRight) partition(arrayHandle, array, smallLeft, smallRight);
	if (bigLeft < bigRight) {
		low = bigLeft;
		high = bigRight;
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
