
#include "Utils/Common.h"
#include "Visualizer.h"

/*
* ALGORITHM INFORMATION:
* Time complexity (worst case) : Unknown
* Time complexity (avg. case)  : O(n * log(n))
* Time complexity (best case)  : O(n * log(n))
* Extra space (worst case)     : O(log(n))
* Extra space (best case)      : O(log(n))
* Type of sort                 : Comparative - Exchange
* Negative integer support     : Yes
*/

typedef struct {
	uintptr_t start;
	uintptr_t n;
	uint8_t badPivot;
} partition_t;

static void InsertionSort(visualizer_array_handle arrayHandle, visualizer_int* array, partition_t part) {
	for (uintptr_t i = part.start + 1; i < part.n; ++i) {
		visualizer_int temp = array[i];
		Visualizer_UpdateRead(arrayHandle, i, 1.0);
		uintptr_t j;
		for (j = i; j > 0; --j) {
			Visualizer_UpdateRead(arrayHandle, j - 1, 1.0);
			if (array[j - 1] <= temp)
				break;
			Visualizer_UpdateWrite(arrayHandle, j, array[j - 1], 1.0);
			array[j] = array[j - 1];
		}
		Visualizer_UpdateWrite(arrayHandle, j, temp, 1.0);
		array[j] = temp;
	}
}

// https://stackoverflow.com/a/55242934/13614676

static inline uintptr_t MedianOf3(
	visualizer_array_handle arrayHandle,
	visualizer_int* array,
	intptr_t left,
	intptr_t right,
	intptr_t mid
) {
	Visualizer_UpdateRead2(arrayHandle, mid, left, 1.0);
	Visualizer_UpdateRead2(arrayHandle, mid, right, 1.0);
	if ((array[mid] > array[left]) ^ (array[mid] > array[right]))
		return mid;
	Visualizer_UpdateRead2(arrayHandle, right, mid, 1.0);
	Visualizer_UpdateRead2(arrayHandle, right, left, 1.0);
	if ((array[right] < array[mid]) ^ (array[right] < array[left]))
		return right;
	return left;
}

static inline uintptr_t MedianOf3Simple(visualizer_array_handle arrayHandle, visualizer_int* array, partition_t part) {
	return MedianOf3(arrayHandle, array, part.start, part.start + part.n - 1, part.start + (part.n / 2));
}

// This method is based on median of medians but it's faster & simpler.
// The worst case partition size is not proved,
// but it can defeat all tested quicksort adversary inputs.

static uintptr_t MedianOf3Recursive(visualizer_array_handle arrayHandle, visualizer_int* array, partition_t part) {
	uintptr_t n = part.n;

	do {
		uintptr_t remSize = (uintptr_t)n % 3;
		n /= 3;

		uintptr_t i;
		uintptr_t iMedian;
		for (i = 0; i < n; ++i) {
			iMedian = MedianOf3(arrayHandle, array, part.start + i * 3, part.start + i * 3 + 1, part.start + i * 3 + 2);
			Visualizer_UpdateSwap(arrayHandle, iMedian, part.start + i, 1.0);
			swap(&array[iMedian], &array[part.start + i]);
		}
		if (remSize > 0) {
			// Since we can't calculate the median of 2, just pick the first value.
			Visualizer_UpdateSwap(arrayHandle, part.start + i * 3, part.start + i, 1.0);
			swap(&array[part.start + i * 3], &array[part.start + i]);
			++n;
		}
	} while (n >= 3);
	return part.start;
}

static uintptr_t GetPivot(visualizer_array_handle arrayHandle, visualizer_int* array, partition_t* part) {
	switch (part->badPivot) {
		case 0:
			return part->start + (part->n / 2);
		case 1:
			return MedianOf3Simple(arrayHandle, array, *part);
		case 2:
			return MedianOf3Simple(arrayHandle, array, *part);
		case 3:
			return MedianOf3Simple(arrayHandle, array, *part);
		case 4:
			part->badPivot = 0;
			return MedianOf3Recursive(arrayHandle, array, *part);
		default:
			ext_unreachable();
	}
}

void ImprovedIntroSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0, Visualizer_SleepScale_NLogN)
	);

	const uintptr_t nSmallest = 16;

	// This static array can be big but if the stack can't hold this
	// then it probably can't hold the worst case of the recursive algorithm either.
	partition_t stack[sizeof(uintptr_t) * 8]; // CHAR_BIT == 8 but who cares
	uint8_t stackSize = 0;
	stack[stackSize++] = (partition_t){ 0, n, 0 };

	do {
		partition_t part = stack[--stackSize];
		while (part.n >= nSmallest) {
			uintptr_t left = part.start;
			uintptr_t right = part.start + part.n - 1;
			uintptr_t pivot = GetPivot(arrayHandle, array, &part);
			visualizer_int pivotValue = array[pivot];

			visualizer_marker pointer = Visualizer_CreateMarker(arrayHandle, pivot, Visualizer_MarkerAttribute_Pointer);
			while (left <= right) {
				while (array[left] < pivotValue) {
					Visualizer_UpdateRead(arrayHandle, left, 1.0);
					++left;
				}
				while (array[right] > pivotValue) {
					Visualizer_UpdateRead(arrayHandle, right, 1.0);
					--right;
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
			// Is slower but reduces stack size in the worst case to log2(n)
			partition_t leftPart = { part.start, right + 1 - part.start, part.badPivot };
			partition_t rightPart = { left, part.start + part.n - left, part.badPivot };
			uint8_t bigLeft = (leftPart.n > rightPart.n);
			partition_t smallPart = bigLeft ? rightPart : leftPart;
			partition_t bigPart = bigLeft ? leftPart : rightPart;

			if (smallPart.n < part.n / 4)
				++bigPart.badPivot;
			else
				bigPart.badPivot = 0;
			smallPart.badPivot = 0;

			if (bigPart.n >= nSmallest)
				stack[stackSize++] = bigPart;
			part = smallPart;
		}
	} while (stackSize > 0);

	InsertionSort(arrayHandle, array, (partition_t){ 0, n });
}
