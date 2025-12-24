
#include "Visualizer.h"

void BinaryInsertion(visualizer_int* array, usize start, usize end) {
	for (usize i = start; i < end; ++i) {
		visualizer_int item = array[i];
		usize low = start;
		usize high = i;

		while (low < high) {
			usize mid = low + ((high - low) / 2); // avoid int overflow!

			// Do NOT move equal elements to right of inserted element.
			// This maintains stability!
			if (item < array[mid])
				high = mid;
			else
				low = mid + 1;
		}

		for (usize j = i; j > low; --j)
			array[j] = array[j - 1];
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

void InsertionSort(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize n) {
	Visualizer_SetAlgorithmSleepMultiplier(
		Visualizer_ScaleSleepMultiplier(n, 1.0f, Visualizer_SleepScale_NN)
	);
	
	if (n < 2) return;
	
	for (usize i = 1; i < n; ++i) {
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
