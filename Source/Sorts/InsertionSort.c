
#include "Visualizer.h"

void BinaryInsertion(visualizer_int* array, intptr_t start, intptr_t end) {
	for (intptr_t i = start; i < end; ++i) {
		visualizer_int item = array[i];
		intptr_t low = start;
		intptr_t high = i;

		while (low < high) {
			intptr_t mid = low + ((high - low) / 2); // avoid int overflow!

			// Do NOT move equal elements to right of inserted element.
			// This maintains stability!
			if (item < array[mid])
				high = mid;
			else
				low = mid + 1;
		}

		for (intptr_t j = i - 1; j >= low; --j)
			array[j + 1] = array[j];
		array[low] = item;
	}
	return;
}

// Exports:

/*
* ALGORITHM INFORMATION:
* Time complexity (worst Case) : O(n ^ 2)
* Time complexity (avg. Case)  : O(n ^ 2)
* Time complexity (best Case)  : O(n)
* Extra space                  : No
* Type of sort                 : Comparative - Insert
* Negative integer support     : Yes
*/

void InsertionSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	if (n < 2) return;

	for (intptr_t i = 1; i < n; ++i) {
		visualizer_int temp = array[i];
		Visualizer_UpdateRead(arrayHandle, i, 1.0);
		intptr_t j;
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
