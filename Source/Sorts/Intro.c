
#include "Utils/Common.h"
#include "Visualizer.h"

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

static uintptr_t MedianOfMedians(visualizer_array_handle arrayHandle, visualizer_int* array, partition_t part) {
	const uintptr_t groupSize = 8;
	uintptr_t n = part.n;

	do {
		uintptr_t remSize = (uintptr_t)n % groupSize;
		n /= groupSize;

		uintptr_t i;
		uintptr_t iNew;
		uintptr_t iOld;
		for (i = 0; i < n; ++i) {
			InsertionSort(arrayHandle, array, (partition_t){ part.start + i * groupSize, groupSize });
			iNew = part.start + i;
			iOld = part.start + i * groupSize + (groupSize / 2);
			Visualizer_UpdateSwap(arrayHandle, iNew, iOld, 2.0);
			swap(&array[iNew], &array[iOld]);
		}
		if (remSize > 0) {
			InsertionSort(arrayHandle, array, (partition_t){ part.start + i * groupSize, remSize });
			iNew = part.start + i;
			iOld = part.start + i * groupSize + (remSize / 2);
			Visualizer_UpdateSwap(arrayHandle, iNew, iOld, 2.0);
			swap(&array[iNew], &array[iOld]);
			++n;
		}
	} while (n >= groupSize);
	return part.start;
}

// https://stackoverflow.com/a/55242934/13614676

static uintptr_t MedianOf3(visualizer_array_handle arrayHandle, visualizer_int* array, partition_t part) {
	uintptr_t left = part.start;
	uintptr_t right = part.start + part.n - 1;
	uintptr_t mid = part.start + (part.n / 2);
	Visualizer_UpdateRead2(arrayHandle, mid, left, 2.0);
	Visualizer_UpdateRead2(arrayHandle, mid, right, 2.0);
	if ((array[mid] > array[left]) ^ (array[mid] > array[right]))
		return mid;
	Visualizer_UpdateRead2(arrayHandle, right, mid, 2.0);
	Visualizer_UpdateRead2(arrayHandle, right, left, 2.0);
	if ((array[right] < array[mid]) ^ (array[right] < array[left]))
		return right;
	return left;
}

static uintptr_t GetPivot(visualizer_array_handle arrayHandle, visualizer_int* array, partition_t* part) {
	switch (part->badPivot) {
		case 0:
			return part->start + (part->n / 2);
		case 1:
			return MedianOf3(arrayHandle, array, *part);
		case 2:
			return MedianOf3(arrayHandle, array, *part);
		case 3:
			part->badPivot = 0;
			return MedianOfMedians(arrayHandle, array, *part);
		default:
			ext_unreachable();
	}
}

void IntroSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0, Visualizer_SleepScale_NLogN)
	);

	const uintptr_t nSmallest = 16;

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
			// Is slower but prevents O(n) call stack in worst case
			partition_t leftPart = { part.start, right + 1 - part.start, part.badPivot };
			partition_t rightPart = { left, part.start + part.n - left, part.badPivot };
			uint8_t bigLeft = (leftPart.n > rightPart.n);
			partition_t smallPart = bigLeft ? rightPart : leftPart;
			partition_t bigPart = bigLeft ? leftPart : rightPart;

			if (smallPart.n < part.n / 4)
				++bigPart.badPivot;

			if (bigPart.n >= nSmallest)
				stack[stackSize++] = bigPart;
			part = smallPart;
		}
	} while (stackSize > 0);

	InsertionSort(arrayHandle, array, (partition_t){ 0, n });
}
