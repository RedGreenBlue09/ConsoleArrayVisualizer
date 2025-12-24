
#include "Visualizer.h"

#include "Utils/GuardedMalloc.h"

static void pigeonholeSort(visualizer_int* array, usize n) {
	visualizer_int Min = array[0];
	visualizer_int Max = Min;

	for (usize i = 1; i < n; ++i) {
		if (array[i] < Min)
			Min = array[i];
		if (array[i] > Max)
			Max = array[i];
	}

	visualizer_int Range = Max - Min + 1;
	if (Range < 2) return;

	usize* Holes = calloc_guarded(Range, sizeof(usize));

	for (usize i = 0; i < n; ++i)
		Holes[array[i] - Min] += 1;

	for (visualizer_int i = 0; i < Range; ++i)
		for (; Holes[i] > 0; --Holes[i])
			*array++ = i + Min;

	free(Holes);
}

// Exports:

/*
* ALGORITHM INFORMATION:
* Time complexity              : O(n + r)
* Extra space                  : O(r)
* Type of sort                 : Distributive
* Negative integer support     : Yes
*/


void PigeonholeSort(usize iThread, visualizer_array arrayHandle, visualizer_int* array, usize n) {
	pigeonholeSort(array, n);
	return;
}
