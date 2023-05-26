
#include "Sorts.h"
#include "Visualizer/Visualizer.h"

void BIS_BinaryInsertion(isort_t* array, intptr_t start, intptr_t end) {

	intptr_t start2 = start;
	intptr_t end2 = end;

	for (intptr_t i = start2; i < end2; ++i) {

		isort_t item = array[i];
		intptr_t low = start2;
		intptr_t high = i;

		while (low < high) {
			intptr_t mid = low + ((high - low) / 2); // avoid int overflow!

			// Do NOT move equal elements to right of inserted element.
			// This maintains stability!
			if (item < array[mid]) {
				high = mid;
			} else {
				low = mid + 1;
			}
		}

		intptr_t j = i - 1;

		while (j >= low) {
			array[j + 1] = array[j];
			--j;
		}
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

void InsertionSort(isort_t* array, intptr_t n) {

	if (n < 2) return;

	for (intptr_t i = 1; i < n; ++i) {
		isort_t temp = array[i];
		intptr_t j = i;
		while ((j > 0) && (array[j - 1] > temp)) {
			array[j] = array[j - 1];
			j -= 1;
		}
		array[j] = temp;
	}
}
