
#include "Visualizer.h"

#include "Utils/GuardedMalloc.h"

static void pigeonholeSort(visualizer_int* array, intptr_t n) {

	intptr_t i;

	intptr_t Min = array[0];
	intptr_t Max = Min;

	for (i = 1; i < n; ++i) {

		if (array[i] < Min)
			Min = array[i];
		if (array[i] > Max)
			Max = array[i];

	}

	intptr_t Range = Max - Min + 1;
	if (Range < 2) return;

	visualizer_int* Holes = malloc_guarded(Range * sizeof(visualizer_int));
	if (Holes == 0) return;

	Holes -= Min;
	Max += 1;

	for (i = Min; i < Max; ++i)
		Holes[i] = 0;

	for (i = 0; i < n; ++i)
		Holes[array[i]] += 1;

	for (i = Min; i < Max; ++i) {

		while (Holes[i] > 0) {
			Holes[i] -= 1;
			*array = (visualizer_int)(i + Min);
			++array;
		}
	}

	free(Holes + Min);
}

// Exports:

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n + r)
* Extra space                  : O(r)
* Type of sort                 : Distributive
* Negative integer support     : Yes
*/


void PigeonholeSort(visualizer_array_handle arrayHandle, visualizer_int* array, intptr_t n) {
	pigeonholeSort(array, (intptr_t)n);
	return;
}
