
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
	usize start;
	usize n;
	uint8_t badPivot;
} partition_t;

static void InsertionSort(usize iThread, visualizer_array arrayHandle, visualizer_int* array, partition_t part) {
	for (usize i = part.start + 1; i < part.n; ++i) {
		visualizer_int temp = array[i];
		Visualizer_UpdateRead(iThread, arrayHandle, i, 1.0f);
		usize j;
		for (j = i; j > 0; --j) {
			Visualizer_UpdateRead(iThread, arrayHandle, j - 1, 1.0f);
			if (array[j - 1] <= temp)
				break;
			Visualizer_UpdateWrite(iThread, arrayHandle, j, array[j - 1], 1.0f);
			array[j] = array[j - 1];
		}
		Visualizer_UpdateWrite(iThread, arrayHandle, j, temp, 1.0f);
		array[j] = temp;
	}
}

// https://stackoverflow.com/a/55242934/13614676

static inline usize MedianOf3(
	usize iThread,
	visualizer_array arrayHandle,
	visualizer_int* array,
	usize left,
	usize mid,
	usize right
) {
	Visualizer_UpdateRead2(iThread, arrayHandle, mid, left, 1.0f);
	Visualizer_UpdateRead2(iThread, arrayHandle, mid, right, 1.0f);
	if ((array[mid] > array[left]) ^ (array[mid] > array[right]))
		return mid;
	Visualizer_UpdateRead2(iThread, arrayHandle, right, left, 1.0f);
	if ((array[right] < array[mid]) ^ (array[right] < array[left]))
		return right;
	return left;
}

static usize MedianOf3Simplest(usize iThread, visualizer_array arrayHandle, visualizer_int* array, partition_t part) {
	return MedianOf3(
		iThread,
		arrayHandle,
		array,
		part.start,
		part.start + (part.n / 2),
		part.start + part.n - 1
	);
}

static usize MedianOf3Simple(usize iThread, visualizer_array arrayHandle, visualizer_int* array, partition_t part) {
	return MedianOf3(
		iThread,
		arrayHandle,
		array,
		part.start + (part.n / 4),
		part.start + (part.n / 2),
		part.start + (part.n / 2) + (part.n / 4)
	);
}

// This method is based on median of medians but it's faster & simpler.
// The worst case partition size is not proved,
// but it can defeat all tested quicksort adversary inputs.

static usize MedianOf3Recursive(usize iThread, visualizer_array arrayHandle, visualizer_int* array, partition_t part) {
	usize n = part.n;

	do {
		usize remSize = n % 3;
		n /= 3;

		usize i;
		usize iMedian;
		for (i = 0; i < n; ++i) {
			iMedian = MedianOf3(iThread, arrayHandle, array, part.start + i * 3, part.start + i * 3 + 1, part.start + i * 3 + 2);
			Visualizer_UpdateSwap(iThread, arrayHandle, iMedian, part.start + i, 1.0f);
			swap(&array[iMedian], &array[part.start + i]);
		}
		if (remSize > 0) {
			// Since we can't calculate the median of 2, just pick the first value.
			Visualizer_UpdateSwap(iThread, arrayHandle, part.start + i * 3, part.start + i, 1.0f);
			swap(&array[part.start + i * 3], &array[part.start + i]);
			++n;
		}
	} while (n >= 3);
	return part.start;
}

static usize GetPivot(usize iThread, visualizer_array arrayHandle, visualizer_int* array, partition_t* part) {
	switch (part->badPivot) {
		case 0:
			return MedianOf3Simplest(iThread, arrayHandle, array, *part);
		case 1:
			return MedianOf3Simplest(iThread, arrayHandle, array, *part);
		case 2:
			return MedianOf3Simple(iThread, arrayHandle, array, *part);
		case 3:
			return MedianOf3Simple(iThread, arrayHandle, array, *part);
		case 4:
			return MedianOf3Recursive(iThread, arrayHandle, array, *part);
		default:
			ext_unreachable();
	}
}

void ImprovedIntroSort(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0f, Visualizer_SleepScale_NLogN)
	);

	const usize nSmallest = 16;

	// This static array can be big but if the stack can't hold this
	// then it probably can't hold the worst case of the recursive algorithm either.
	partition_t stack[sizeof(usize) * 8]; // CHAR_BIT == 8 but who cares
	uint8_t stackSize = 0;
	stack[stackSize++] = (partition_t){ 0, n, 0 };

	do {
		partition_t part = stack[--stackSize];
		while (1) {
			isize left = part.start;
			isize right = part.start + part.n - 1;
			isize pivot = GetPivot(iThread, arrayHandle, array, &part);
			visualizer_int pivotValue = array[pivot];

			visualizer_marker pointer = Visualizer_CreateMarker(iThread, arrayHandle, pivot, Visualizer_MarkerAttribute_Pointer);
			while (left <= right) {
				Visualizer_UpdateRead(iThread, arrayHandle, left, 1.0f);
				while (array[left] < pivotValue) {
					++left;
					Visualizer_UpdateRead(iThread, arrayHandle, left, 1.0f);
				}
				Visualizer_UpdateRead(iThread, arrayHandle, right, 1.0f);
				while (array[right] > pivotValue) {
					--right;
					Visualizer_UpdateRead(iThread, arrayHandle, right, 1.0f);
				}

				if (left <= right) {
					Visualizer_UpdateSwap(iThread, arrayHandle, left, right, 1.0f);
					swap(&array[left], &array[right]);
					++left;
					--right;
				}
			}
			Visualizer_RemoveMarker(iThread, pointer);

			// Call tail optimization
			// Is slower but reduces stack size in the worst case to log2(n)
			partition_t leftPart = { part.start, right + 1 - part.start, part.badPivot };
			partition_t rightPart = { left, part.start + part.n - left, part.badPivot };
			uint8_t bigLeft = (leftPart.n > rightPart.n);
			partition_t* smallPart = bigLeft ? &rightPart : &leftPart;
			partition_t* bigPart = bigLeft ? &leftPart : &rightPart;

			if (smallPart->n < part.n / 4)
				++bigPart->badPivot;
			else
				bigPart->badPivot = 0;
			smallPart->badPivot = 0;

			if (bigPart->n >= nSmallest)
				stack[stackSize++] = *bigPart;
			if (smallPart->n >= nSmallest)
				part = *smallPart;
			else
				break;
		}
	} while (stackSize > 0);

	InsertionSort(iThread, arrayHandle, array, (partition_t){ 0, n });
}
